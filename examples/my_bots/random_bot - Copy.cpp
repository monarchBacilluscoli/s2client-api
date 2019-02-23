#include"random_bot.h"
#include<numeric>
#include<random>

namespace sc2 {
	// 声明别名
	using command = std::tuple<const Unit*, Point2D, const Unit*>;
	using basic_solution = std::vector<command>;
	using population = std::vector<std::pair<basic_solution, float>>;

	//? test of the cooldown time
	const Unit* m_test_unit;

	void random_bot::OnGameStart() {
		////? test of the cooldown time
		//m_test_unit = Observation()->GetUnits(Unit::Alliance::Self)[0];
		// 初始化部分数据
		m_alive_self_units = Observation()->GetUnits(Unit::Alliance::Self);
		m_marine = Observation()->GetUnitTypeData().at(int(UNIT_TYPEID::TERRAN_MARINE));

		//? test of the map_data
		//std::string map_data = Observation()->GetGameInfo().terrain_height.data;
		//for (auto& c:map_data) {
		//	std::cout << (int)c << '\t'; //todo figure out what it is
		//}
		
		
	}
	//basic_solution::basic_solution(const std::vector<std::pair<Point2D, Unit*>>& c):m_command(c) {}
	//void basic_solution::set_command(const std::vector<std::pair<Point2D, Unit*>>& c) {
	//	m_command = c;
	//}
	//void basic_solution::set_object(float f) {
	//	m_fitness = f;
	//}
	//std::pair<Point2D,Unit*> basic_solution::operator[](int i) {
	//	return m_command.at(i);
	//}
	//void basic_solution::resize(int n) {
	//	m_command.resize(n);
	//}
	//float basic_solution::fitness() {
	//	return m_fitness;
	//}
	void random_bot::OnStep() {
		////? test of the cooldown time
		//Debug()->DebugTextOut(std::to_string(m_test_unit->weapon_cooldown));
		//std::cout << Observation()->GetGameLoop() << ":";
		//float cooldown = m_test_unit->weapon_cooldown;
		//if (cooldown - 0.f < 1e-7) {
		//	std::cout << cooldown << std::endl;
		//}
		//else {
		//	std::cout << cooldown << '\t';
		//}
		//

		// 更新游戏数据
		m_alive_self_units = Observation()->GetUnits(Unit::Alliance::Self);
		m_alive_enemy_units = Observation()->GetUnits(Unit::Alliance::Enemy);
		m_all_alive_units = Observation()->GetUnits();
		m_game_info = Observation()->GetGameInfo();

		//! Debug部分
		// 输出线框
		display_units_weapon_range(m_all_alive_units);
		display_units_collision_range(m_all_alive_units);
		display_solution_line(m_selected_solution);
		Debug()->SendDebug();

		//// 算法部分
		if (Observation()->GetGameLoop() % m_frames_per_deploy == 0) {
			// 生成随机解
			generate_random_solution();
			//generate_attack_nearest_without_move_solution();
			// 评估所有解
			evaluate_all_solutions();
			// 选择解
			m_selected_solution = select_solution();
			//basic_solution selected_solution = select_random_solution();
			// 部署
			deploy_solution(m_selected_solution);
		}
		//! Debug部分
		

		
		////? test of the svae/load
		//if (Observation()->GetGameLoop() % 500 == 0) {
		//	if (m_is_save) {
		//		Control()->Load();
		//	}
		//	else {
		//		Control()->Save();
		//		m_is_save = true;
		//	}
		//}
		////? test of enemy control
		//Debug()->DebugEnemyControl();
		//Units enemies = Observation()->GetUnits(Unit::Alliance::Enemy);
		//Actions()->UnitCommand(enemies, ABILITY_ID::MOVE, Point2D(10, 10));
	}

	void random_bot::OnUnitIdle(const Unit * unit) {
		std::vector<Weapon> weapons = Observation()->GetUnitTypeData()[unit->unit_type].weapons;
		const Unit* target = search_nearest_unit_from_point(unit->pos, Unit::Alliance::Enemy, [weapons](const Unit& unit) ->bool {
			bool can_attack = false;
			for (Weapon w : weapons) {
				switch (w.type) {
				case Weapon::TargetType::Any:
					return true;
					break;
				default:
					if (((int)w.type - 1) == int(unit.is_flying)) {
						return true;
					}
					else {
						return false;
					}
				}
			}
		});
		Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, target);
	}

	void random_bot::generate_random_solution() {
		m_population.clear();
		int alive_allies_number = m_alive_self_units.size();
		for (size_t i = 0; i < m_population_size; i++) {
			// 生成一个可行解
			basic_solution bs = basic_solution();
			for (size_t j = 0; j < alive_allies_number; j++) {
				// 为每一个单位生成一个操作
				const UnitTypeData unit_type = Observation()->GetUnitTypeData()[m_alive_self_units[j]->unit_type];
				Point2D random_offset = generate_random_point_in_circle(unit_type.movement_speed*m_frames_per_deploy / 24);
				Point2D random_pos = m_alive_self_units[j]->pos + random_offset;
				Units enemy_within_firerange = search_units_within_radius(m_alive_self_units[j]->pos, Unit::Alliance::Enemy, unit_type.weapons[0].range);
				const Unit* target_enemy = nullptr;
				if (enemy_within_firerange.size()) { // 如果有敌人在攻击范围内
					target_enemy = select_lowest_hp_enemy(enemy_within_firerange);
				}
				else { //这一句让算法扫描了最近的单位
					//target_enemy = search_nearest_unit_from_point(m_alive_self_units[j]->pos, Unit::Alliance::Enemy);
				}

				command t_command = std::make_tuple(m_alive_self_units[j], random_pos, target_enemy);
				bs.push_back(t_command);
			}
			// 插入vector
			m_population.push_back(std::pair<basic_solution, float>(bs, -FLT_MAX));
		}
	}

	void random_bot::generate_attack_nearest_without_move_solution() {
		m_population.clear();
		int alive_allies_number = m_alive_self_units.size();
		basic_solution solution;
		// 实际上仅需生成一个解
		for (size_t j = 0; j < alive_allies_number; j++) {
			UnitTypeData u_type = Observation()->GetUnitTypeData()[m_alive_self_units[j]->unit_type];
			const Unit* target = search_nearest_unit_from_point(m_alive_self_units[j]->pos, Unit::Alliance::Enemy);
			if (Distance2D(target->pos, m_alive_enemy_units[j]->pos) > u_type.weapons[0].range) {
				target = nullptr;
			}
			command command = { m_alive_self_units[j],m_alive_self_units[j]->pos, target };
			solution.push_back(command);
		}
		for (size_t i = 0; i < m_population_size; i++) {
			m_population.push_back(std::pair<basic_solution, float>(solution, -FLT_MAX));
		}
	}

	std::vector<basic_solution> random_bot::cross_over(const basic_solution& a, const basic_solution& b) {
		std::vector<basic_solution> children;
		children.push_back(a);
		children.push_back(b);
		int cross_over_point = GetRandomInteger(0, a.size());
		children[0][cross_over_point] = b[cross_over_point];
		children[1][cross_over_point] = a[cross_over_point];
		return children;
	}

	basic_solution random_bot::mutate(const basic_solution & s, float random_ratio_of_command, float random_range) {
		basic_solution mutated_s = s;
		int random_commands_number = s.size()*random_ratio_of_command;
		std::vector<int> sub(s.size());
		std::iota(std::begin(sub), std::end(sub), 0);
		std::random_device rd;
		std::mt19937 g(rd());
		shuffle(std::begin(sub), std::end(sub), g);
		for (size_t i = 0; i < random_commands_number; i++) {
			Point2D radom_offset(GetRandomScalar()*random_range, GetRandomScalar()*random_range);
			std::get<1>(mutated_s[sub[i]]) += radom_offset;
		}
		return mutated_s;
	}

	void random_bot::evaluate_all_solutions() {
		m_hurt_objective.resize(m_population.size());
		for (size_t i = 0; i < m_population_size; i++) {
			m_population[i].second = evaluate_single_solution_attack(m_population[i].first);
			m_hurt_objective[i] = evaluate_single_solution_hurt(m_population[i].first);
		}
	}

	basic_solution random_bot::select_solution() {
		float highest_fitness = m_population[0].second - m_hurt_objective[0];
		basic_solution selected_solution = m_population[0].first;
		int index(0);
		for (size_t i = 0; i < m_population.size(); i++) {
			if (m_population[i].second - m_hurt_objective[i] > highest_fitness) {
				selected_solution = m_population[i].first;
				index = i;
			}
		}
		Debug()->DebugTextOut("obj1: " + std::to_string(m_population[index].second) + "\t");
		Debug()->DebugTextOut("obj2: " + std::to_string(m_hurt_objective[index]));
		Debug()->SendDebug();
		std::cout << "obj1:" << m_population[index].second << "\t" << "obj2:" << m_hurt_objective[index] << std::endl;
		return selected_solution;
	}

	basic_solution random_bot::select_random_solution() {
		return GetRandomEntry<population>(m_population).first;
	}

	

	void random_bot::deploy_solution(const basic_solution & s) {
		for (const command& c : s) {
			if (std::get<0>(c)) {
				// 先部署移动
				Actions()->UnitCommand(std::get<0>(c), ABILITY_ID::MOVE, std::get<1>(c), false); // the first move command must be deployed 
				// 再部署攻击
				if (std::get<2>(c)) {
					bool wait_for_last_command_finish = true;
					/*if (Distance2D(std::get<0>(c)->pos, std::get<2>(c)->pos) <= m_marine.weapons[0].range) {
						wait_for_last_command_finish = false;
					}*/
					Actions()->UnitCommand(std::get<0>(c), ABILITY_ID::ATTACK, std::get<2>(c), wait_for_last_command_finish);
				}
			}
		}
	}

	float random_bot::evaluate_single_solution_attack(const basic_solution & s) {
		float total_damage_make(0);
		std::map<const Unit*, float> damage_to_enemy;
		for (auto c : s) {
			const Unit* enemy = std::get<2>(c);
			if (enemy!=nullptr) {
				bool is_within_range = Distance2D(std::get<1>(c), enemy->pos) < m_marine.weapons[0].range;
				if (is_within_range) {
					damage_to_enemy[enemy] += damage_weapon_to_unit(m_marine.weapons[0], enemy);
				}
			}
		}
		// 处理过量伤害
		for (auto& d : damage_to_enemy) {
			if (d.second > d.first->health) {
				total_damage_make += d.first->health;
			}
			total_damage_make += d.second;
		}
		return total_damage_make;
	}

	float random_bot::evaluate_single_solution_hurt(const basic_solution & s) {
		float total_hurt(0);
		Units enemy_units = Observation()->GetUnits(Unit::Alliance::Enemy);
		for (const Unit* u : enemy_units) {
			Weapon u_weapon = Observation()->GetUnitTypeData()[u->unit_type].weapons[0];
			Units possible_targets = search_units_within_radius(u->pos, Unit::Alliance::Self, u_weapon.range); //todo: 此处有问题，pos不对，应该是转换后的位置
			const Unit* target(nullptr);
			if (!possible_targets.empty()) {
				target = GetRandomEntry(possible_targets);
				total_hurt += damage_weapon_to_unit(u_weapon, target);
			}
		}
		return total_hurt;
	}

	Point2D random_bot::generate_random_point_in_circle(float r) {
		float random_r = GetRandomFraction()*r;
		float theta = GetRandomFraction() * 2 * PI;
		float x = random_r * cos(theta);
		float y = random_r * sin(theta);
		return Point2D(x, y);
	}

	Units random_bot::search_units_within_radius(const Point2D& p, Unit::Alliance a, float r) {
		Units us = Observation()->GetUnits(a, [r, p](const Unit& u) {
			if (Distance2D(u.pos, p) <= r)
				return true;
			else
				return false;
		});
		return us;
	}

	const Unit * random_bot::search_nearest_unit_from_point(const Point2D & p, Unit::Alliance a, Filter filter) {
		Units us = Observation()->GetUnits(a, filter);
		return select_nearest_unit(p, us);
	}

	const Unit * random_bot::select_lowest_hp_enemy(const Units& us) {
		float lowest_hp = FLT_MAX;
		const Unit* selected_unit = nullptr;
		for (const Unit* u : us) {
			if (u->health < lowest_hp) {
				selected_unit = u;
			}
		}
		return selected_unit;
	}

	const Unit * random_bot::select_nearest_unit(const Point2D& p, const Units & us) {
		float min_distance = FLT_MAX;
		const Unit* selected_unit = nullptr;
		float dis;
		for (const auto u : us) {
			dis = Distance2D(p, u->pos);
			if (dis < min_distance) {
				selected_unit = u;
				min_distance = dis;
			}
		}
		return selected_unit;
	}

	float random_bot::damage_weapon_to_unit(const Weapon& w, const Unit * u) {
		float damage = 0;
		UnitTypeData u_type = Observation()->GetUnitTypeData()[u->unit_type];
		if ((int)(w.type) - 1 == u->is_flying || w.type == Weapon::TargetType::Any) {
			float bonus_damage = 0;
			for (auto i : w.damage_bonus) {
				std::vector<Attribute>::iterator result = std::find(u_type.attributes.begin(), u_type.attributes.end(), i.attribute);
				if (result != u_type.attributes.end()) {
					bonus_damage += i.bonus;
				}
			}
			damage += (w.damage_ - u_type.armor + bonus_damage)*w.attacks;
			return damage;
		}
	}

	void random_bot::display_units_weapon_range(Units us) {
		for (auto u : us) {
			std::vector<Weapon> weapons = Observation()->GetUnitTypeData()[u->unit_type].weapons;
			Color sphere_color = Colors::Blue;
			if (u->alliance == Unit::Alliance::Self || u->alliance == Unit::Alliance::Ally) {
				sphere_color = Colors::Red;
			}
			for (auto& w : weapons) {
				Debug()->DebugSphereOut(u->pos, w.range, sphere_color);
			}
		}
	}

	void random_bot::display_units_collision_range(Units us) {
		for (auto u : us) {
			Debug()->DebugSphereOut(u->pos, u->radius);
		}
	}

	void random_bot::display_map_grid(const GameInfo& info) {
		
	}

	void random_bot::display_units_actions() {
	}

	void random_bot::display_units_move_action(Units us) {
		for (auto u : us) {
			if (!(u->orders.empty())&&u->orders[0].ability_id==ABILITY_ID::MOVE) {
				//todo there must be something to do
			}
		}
	}

	void random_bot::display_solution_line(const basic_solution & bs) {
		if (!bs.empty()) {
			for (const command& c : bs) {
				Debug()->DebugLineOut(std::get<0>(c)->pos, Point3D(std::get<1>(c).x, std::get<1>(c).y, std::get<0>(c)->pos.z));
			}
		}
	}

	//population random_search::generate_random_solutions(const ObservationInterface * ob) {
	//	return population();
	//}
	//std::pair<float, float> random_search::generate_random_position_in_circle(float r) {
	//	float random_radius = GetRandomFraction() * r;
	//	return std::pair<float,float>(cos(random_radius), sin(random_radius));
	//}
}