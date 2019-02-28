#include "one_frame_bot.h"

namespace sc2 {
	using solution = std::map<Tag, RawActions>;
	using population = std::vector<solution>;

	Units one_frame_bot::search_units_within_radius(const Point2D & p, float r, Unit::Alliance a) {
		Units us = Observation()->GetUnits(a, [r, p](const Unit& u) {
			if (Distance2D(u.pos, p) <= r)
				return true;
			else
				return false;
		});
		return us;
	}
	Point2D one_frame_bot::generate_random_point_within_radius(float r) {
		float random_r = GetRandomFraction()*r;
		float theta = GetRandomFraction() * 2 * PI;
		float x = random_r * cos(theta);
		float y = random_r * sin(theta);
		return Point2D(x, y);
	}
	const Unit * one_frame_bot::search_nearest_unit_from_point(const Point2D & p, Unit::Alliance a, Filter f) {
		Units us = Observation()->GetUnits(a, f);
		return select_nearest_unit_from_point_(p, us);
	}
	const Unit * one_frame_bot::select_lowest_hp_enemy(const Units & us) {
		float lowest_hp = FLT_MAX;
		const Unit* selected_unit = nullptr;
		for (const Unit* u : us) {
			if (u->health < lowest_hp) {
				selected_unit = u;
			}
		}
		return selected_unit;
	}
	const Unit * one_frame_bot::select_nearest_unit_from_point_(const Point2D & p, const Units & us) {
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
	float one_frame_bot::damage_weapon_to_unit(const Weapon & w, const Unit * u) {
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
	void one_frame_bot::display_fire_range(const Units & us) {
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
	void one_frame_bot::display_units_collision_range(const Units & us) {
		for (auto u : us) {
			Debug()->DebugSphereOut(u->pos, u->radius);
		}
	}
	void one_frame_bot::display_units_move_action(const Units & us) {
		for (auto u : us) {
			if (!(u->orders.empty()) && u->orders[0].ability_id == ABILITY_ID::MOVE) {
				//todo there must be something to do
			}
		}
	}
	void one_frame_bot::OnGameStart() {
		// initialize some data
		m_alive_self_units = Observation()->GetUnits(Unit::Alliance::Self);
		m_unit_types = Observation()->GetUnitTypeData();
		m_marine = m_unit_types[static_cast<int>(UNIT_TYPEID::TERRAN_MARINE)];
	}
	void one_frame_bot::OnStep() {
		// 更新游戏数据
		m_alive_self_units = Observation()->GetUnits(Unit::Alliance::Self);
		m_alive_enemy_units = Observation()->GetUnits(Unit::Alliance::Enemy);
		m_all_alive_units = Observation()->GetUnits();
		m_game_info = Observation()->GetGameInfo();

		//! Debug部分
		// 输出线框
		display_fire_range(m_all_alive_units);
		display_units_collision_range(m_all_alive_units);
		display_solution_line(m_selected_solution);
		Debug()->SendDebug();

		//// 算法部分
		if (Observation()->GetGameLoop() % m_frames_per_deploy == 0) {
			// 生成随机解
			generate_random_solution();
			//generate_attack_nearest_without_move_solution();
			// 评估所有解
			evaluate_all_solutions(m_population, m_damage_objective, m_hurt_objective);
			// 对解进行排序
			sort_solutions(m_population, m_damage_objective, m_hurt_objective);
			for (size_t i = 0; i < produce_times; i++) {
				population children;
				for (size_t i = 0; i < m_crossover_rate*m_population_size / 2; i++) {
					population instant_children = cross_over(GetRandomEntry(m_population), GetRandomEntry(m_population));
					children.insert(std::end(children), std::begin(instant_children), std::end(instant_children));
				}
				m_population.insert(std::end(m_population), std::begin(children), std::end(children));
				// 评估所有解
				evaluate_all_solutions(m_population, m_damage_objective, m_hurt_objective);
				// 对解进行排序
				sort_solutions(m_population, m_damage_objective, m_hurt_objective);
				// 留下部分解
				m_population.erase(std::begin(m_population) + m_population_size, std::end(m_population));
				m_hurt_objective.erase(std::begin(m_hurt_objective) + m_population_size, std::end(m_hurt_objective));
				m_damage_objective.erase(std::begin(m_damage_objective) + m_population_size, std::end(m_damage_objective));
			}
			// 选择解
			m_selected_solution = select_one_solution(m_population, m_damage_objective, m_hurt_objective);
			//basic_solution selected_solution = select_random_solution();
			// 部署
			deploy_solution(m_selected_solution);
		}
	}
	void one_frame_bot::evaluate_all_solutions(const population & p, std::vector<float>& total_damage, std::vector<float>& total_hurt) {
		m_hurt_objective.resize(p.size());
		m_damage_objective.resize(p.size());
		for (size_t i = 0; i < p.size(); i++) {
			total_damage[i] = evaluate_single_solution_damage(m_population[i]);
			total_hurt[i] = evaluate_single_solution_hurt(m_population[i]);
		}
	}
	void one_frame_bot::sort_solutions(population & p, std::vector<float>& total_damage, std::vector<float>& total_hurt) {
		// 对两个目标值逐个相减
		std::vector<float> final_objective(total_damage.size());
		std::transform(std::begin(total_damage), std::end(total_damage), std::begin(total_hurt), std::begin(final_objective), std::minus<float>());
		//todo 此处需要优化
		std::vector<std::pair<int, float>> index_to_obj;
		for (size_t i = 0; i < final_objective.size(); i++) {
			index_to_obj.push_back(std::pair<int, float>(i, final_objective[i]));
		}
		std::sort(std::begin(index_to_obj), std::end(index_to_obj), [](const std::pair<int, float> a, const std::pair<int, float> b)->bool {return a.second > b.second; });
		// 通过上面计算的序进行交换
		population p_(p);
		std::vector<float> td_(total_damage);
		std::vector<float> th_(total_hurt);
		for (size_t i = 0; i < p.size(); i++) {
			int index = index_to_obj[i].first;
			p[i] = p_[index];
			total_damage[i] = td_[index];
			total_hurt[i] = th_[index];
		}
	}
	solution one_frame_bot::select_one_solution(const population & p, std::vector<float>& d, std::vector<float>& h) {
		float highest_fitness = m_damage_objective[0] - m_hurt_objective[0];
		solution selected_solution = m_population[0];
		int index(0);
		for (size_t i = 0; i < m_population.size(); i++) {
			if (m_damage_objective[i] - m_hurt_objective[i] > highest_fitness) {
				selected_solution = m_population[i];
				index = i;
			}
		}
		Debug()->DebugTextOut("obj1: " + std::to_string(m_damage_objective[index]) + "\t");
		Debug()->DebugTextOut("obj2: " + std::to_string(m_hurt_objective[index]));
		Debug()->SendDebug();
		std::cout << "obj1:" << m_damage_objective[index] << "\t" << "obj2:" << m_hurt_objective[index] << std::endl;
		return selected_solution;
	}
}