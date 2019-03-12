#include "one_frame_bot.h"
#include<map>
#include<string>
#include<thread>
#include<mutex>
#include<chrono>
#include"utilities/Point2DPolar.h"

namespace sc2 {
	using command = std::tuple<Tag, RawActions>;
	using solution = std::vector<command>;
	using population = std::vector<solution>;

	std::mutex evaluation_mutex;

	Units one_frame_bot::search_units_within_radius_in_solution(const Point2D& p, float r, const solution& s) {
		Units units_within_radius;
		for (const command& c : s) {
			const Unit* u = get_execution_unit(c);
			switch (static_cast<ABILITY_ID>(get_action(c, 0).ability_id)) {
				// Abilities can move a unit
			case ABILITY_ID::MOVE:
				if (Distance2D(p, get_action(c, 0).target_point) - u->radius <= r) {
					units_within_radius.push_back(u);
				}
				// Abilities can not move a unit
			case ABILITY_ID::ATTACK:
			default:
				if (Distance2D(p, u->pos) - u->radius <= r) {
					units_within_radius.push_back(u);
				}
				break;
			}
		}
		return units_within_radius;
	}

	Units one_frame_bot::search_units_can_be_attacked_by_unit_in_solution(const Unit* u, const solution& s) {
		Units units_can_be_attacked;
		for (const command& c : s) {
			const Unit* target_u = get_execution_unit(c);
			if (!get_actions(c).empty()) {
				if (get_action(c, 0).ability_id == ABILITY_ID::MOVE) {
					Point2D target_new_pos = calculate_pos_next_frame(target_u, get_action(c, 0).target_point);
					// for each weapon, to find witch weapon can attack the target enemy
					for (Weapon w : m_unit_types[target_u->unit_type].weapons) {
						if (Distance2D(target_new_pos, u->pos) < w.range + u->radius + target_u->radius && is_weapon_match_unit(w, target_u)) {
							units_can_be_attacked.push_back(target_u);
						}
					}
				}
				else continue; //! there are many other aibilities can move a units
			}
			else {
				throw("there is an empty command@one_fame_bot::search_units_can_be_attacked_by_unit_in_solution");
			}
		}
		return units_can_be_attacked;
	}

	Units one_frame_bot::serach_units_can_be_attacked_by_unit(const Unit* u, Unit::Alliance a) {
		Units units_can_be_attacked;
		// get the longest weapon of u and search units within its range
		std::vector<Weapon> weapons = m_unit_types[u->unit_type].weapons;
		Weapon longest = get_longest_range_weapon_of_weapons(weapons);
		units_can_be_attacked = search_units_within_radius(u->pos, longest.range, a);
		// check if every unit of them can be attacked by each weapon (range & type)
		for (Units::iterator i = units_can_be_attacked.begin(); i < units_can_be_attacked.end(); i++) {
			if (!is_attackable(u, *i)) {
				units_can_be_attacked.erase(i);
			}
		}
		return units_can_be_attacked;
	}

	//? this must be modified, because it don't have to search the center of a unit
	Units one_frame_bot::search_units_within_radius(const Point2D& p, float r, Unit::Alliance a) {
		Units us = Observation()->GetUnits(a, [&p, r](const Unit & u) {
			if (Distance2D(u.pos, p) <= r + u.radius)
				return true;
			else
				return false;
		});
		return us;
	}
	Point2D one_frame_bot::generate_random_point_within_radius(float r) {
		Point2DInPolar pp(GetRandomFraction() * r, GetRandomFraction() * 2 * PI);
		return pp.toPoint2D();
	}
	const Unit* one_frame_bot::search_nearest_unit_from_point(const Point2D & p, Unit::Alliance a, Filter f) {
		Units us = Observation()->GetUnits(a, f);
		return select_nearest_unit_from_point_(p, us);
	}
	const Unit* one_frame_bot::select_lowest_hp_enemy(const Units & us) {
		float lowest_hp = FLT_MAX;
		const Unit* selected_unit = nullptr;
		for (const Unit* u : us) {
			if (u->health < lowest_hp) {
				selected_unit = u;
			}
		}
		return selected_unit;
	}
	const Unit* one_frame_bot::select_nearest_unit_from_point_(const Point2D & p, const Units & us) {
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
		if (is_weapon_match_unit(w, u)) {
			float bonus_damage = 0;
			for (auto i : w.damage_bonus) {
				std::vector<Attribute>::iterator result = std::find(u_type.attributes.begin(), u_type.attributes.end(), i.attribute);
				if (result != u_type.attributes.end()) {
					bonus_damage += i.bonus;
				}
			}
			damage += (w.damage_ - u_type.armor + bonus_damage) * w.attacks;
			return damage;
		}
		else {
			throw("this weapon doesn't match the unit@damage_weapon_to_unit");
		}
	}
	float one_frame_bot::damage_unit_to_unit_without_considering_distance(const Unit * attacking_u, const Unit * target_u) {
		//! just consider the case that every unit only has only one weapon can be used to attack a specific enemy unit
		Weapon w = get_matched_weapons_without_considering_distance(attacking_u, target_u)[0];
		return damage_weapon_to_unit(w, target_u);
	}
	float one_frame_bot::damage_unit_to_unit(const Unit * attacking_u, const Unit * target_u) {
		float dis = real_distance_between_two_units(attacking_u, target_u);
		std::vector<Weapon> weapons = m_unit_types[attacking_u->unit_type].weapons;
		float damage = 0;
		for (const Weapon& w : weapons) {
			// use the distance and weapon feature to check
			if (w.range >= dis || is_weapon_match_unit(w, target_u)) {
				if (damage < damage_weapon_to_unit(w, target_u)) {
					damage = damage_weapon_to_unit(w, target_u);
				}
			}
		}
		return damage;
	}
	float one_frame_bot::basic_movement_one_frame(const UnitTypeData & ut) {
		return ut.movement_speed / m_frames_per_second;
	}
	Point2D one_frame_bot::calculate_pos_next_frame(const Unit * u, const Point2D & p) {
		Point2D new_pos;
		float longest_movement_one_frame = basic_movement_one_frame(m_unit_types[u->unit_type]);
		float two_points_distance = Distance2D(p, u->pos);
		if (two_points_distance < longest_movement_one_frame) {
			new_pos = p;
		}
		else {
			new_pos = (p - u->pos) / two_points_distance * longest_movement_one_frame;
		}
		return new_pos;
	}
	float one_frame_bot::real_distance_between_two_units(const Unit * u1, const Unit * u2) {
		return Distance2D(u1->pos, u2->pos) - u1->radius - u2->radius;
	}
	bool one_frame_bot::is_weapon_match_unit(const Weapon & w, const Unit * u) {
		if ((int)(w.type) - 1 == u->is_flying || w.type == Weapon::TargetType::Any) {
			return true;
		}
		return false;
	}

	std::vector<Weapon> one_frame_bot::get_matched_weapons_without_considering_distance(const Unit * attack, const Unit * target) {
		std::vector<Weapon> matched_weapons;
		for (const Weapon& w : m_unit_types[attack->unit_type].weapons) {
			if (is_weapon_match_unit(w, target)) {
				matched_weapons.push_back(w);
			}
		}
		return matched_weapons;
	}

	Weapon one_frame_bot::get_longest_range_weapon_of_unit_type(const UnitTypeData & ut) {
		if (ut.weapons.empty()) {
			throw("There is no weapon in this unit type@one_frame_bot::get_longest_range_weapon_of_unit_type()");
		}
		else {
			Weapon longest_weapon = ut.weapons[0];
			for (const Weapon& w : ut.weapons) {
				if (longest_weapon.range < w.range) {
					longest_weapon = w;
				}
			}
			return longest_weapon;
		}
	}

	Weapon one_frame_bot::get_longest_range_weapon_of_weapons(const std::vector<Weapon> ws) {
		if (!ws.empty()) {
			if (ws.size() == 1) {
				return ws[0];
			}
			else {
				Weapon longest_range_weapon = ws[0];
				for (const Weapon& w : ws) {
					if (w.range > longest_range_weapon.range) {
						longest_range_weapon = w;
					}
				}
				return longest_range_weapon;
			}
		}
		else {
			throw("no weapon here@one_frame_bot::get_longest_range_weapon_of_weapons");
		}
	}

	bool one_frame_bot::is_attackable(const Unit * attacking_u, const Unit * target_u) {
		std::vector<Weapon> matched_weapons = get_matched_weapons_without_considering_distance(attacking_u, target_u);
		float range = get_longest_range_weapon_of_weapons(matched_weapons).range;
		if (range > real_distance_between_two_units(attacking_u, target_u)) {
			return true;
		}
		return false;
	}

	float one_frame_bot::threat_from_unit_to_unit(const Unit * source_u, const Unit * target_u) {
		float threat = 0;
		float distance = Distance2D(source_u->pos, target_u->pos);
		float zero_potential_field_dis = calculate_zero_potential_field_distance(source_u, target_u);
		if (zero_potential_field_dis != 0) {
			threat = distance / zero_potential_field_dis;
			if (threat > 1) {
				threat = 0;
			}
		}
		return threat * damage_unit_to_unit_without_considering_distance(source_u, target_u);
	}

	float one_frame_bot::threat_from_unit_to_unit_new_pos(const Unit * source_u, const Unit * target_u, const Point2D & pos) {
		float threat = 0;
		float distance = Distance2D(source_u->pos, pos);
		float zero_potential_field_dis = calculate_zero_potential_field_distance(source_u, target_u);
		if (zero_potential_field_dis != 0) {
			threat = distance / zero_potential_field_dis;
			if (threat > 1) {
				threat = 0;
			}
		}
		return threat * damage_unit_to_unit_without_considering_distance(source_u, target_u);
	}

	float one_frame_bot::threat_from_units_to_unit(const Units & source_us, const Unit * target_u) {
		float threat = 0.f;
		//todo try scalar sum
		for (const Unit* u : source_us) {
			threat += threat_from_unit_to_unit(u, target_u);
		}
		return threat;
	}
	float one_frame_bot::threat_from_units_to_unit_new_pos(const Units & source_us, const Unit * target_u, const Point2D & pos) {
		float threat = 0.f;
		for (const Unit* u : source_us) {
			threat += threat_from_unit_to_unit_new_pos(u, target_u, pos);
		}
		return threat;
	}
	float one_frame_bot::calculate_zero_potential_field_distance(const Unit * source_u, const Unit * target_u) {
		float dis;
		std::vector<Weapon> source_weapons = get_matched_weapons_without_considering_distance(source_u, target_u);
		std::vector<Weapon> target_weapons = get_matched_weapons_without_considering_distance(target_u, source_u);

		if (!source_weapons.empty() && !target_weapons.empty()) {
			float target_range = target_weapons[0].range;
			float source_range = source_weapons[0].range;
			/*
			* for hit-and-run
			*/
			if (source_range < target_range) {
				dis = 0.7f * target_range + 0.3f * source_range + source_u->radius;
			}
			/*
			* for run
			*/
			else {
				dis = source_range + source_u->radius + target_u->radius;
			}
		}
		else {
			return 0;
		}
		return dis;
	}
	const Unit* one_frame_bot::get_execution_unit(const command & c) {
		return Observation()->GetUnit(std::get<0>(c));
	}
	const ActionRaw& one_frame_bot::get_action(const command & c, int i) {
		return std::get<1>(c)[i];
	}

	ActionRaw& one_frame_bot::get_action(command & c, int i) {
		return std::get<1>(c)[i];
	}

	const RawActions& one_frame_bot::get_actions(const command & c) {
		return std::get<1>(c);
	}

	RawActions& one_frame_bot::get_actions(command & c) {
		return std::get<1>(c);
	}

	void one_frame_bot::display_fire_range(DebugInterface * debug, const Units & us) {
		for (auto u : us) {
			std::vector<Weapon> weapons = Observation()->GetUnitTypeData()[u->unit_type].weapons;
			Color sphere_color = Colors::Blue;
			if (u->alliance == Unit::Alliance::Self || u->alliance == Unit::Alliance::Ally) {
				sphere_color = Colors::Red;
			}
			for (auto& w : weapons) {
				debug->DebugSphereOut(u->pos, w.range, sphere_color);
			}
		}
	}
	void one_frame_bot::display_units_collision_range(DebugInterface * debug, const Units & us) {
		for (auto u : us) {
			debug->DebugSphereOut(u->pos, u->radius);
		}
	}
	void one_frame_bot::display_units_pos(DebugInterface * debug, const Units & us) {
		for (auto u : us) {
			//if (u->is_selected) {
			std::string pos_info;
			pos_info = std::to_string(u->pos.x) + ", " + std::to_string(u->pos.y) + "," + std::to_string(u->pos.z);
			debug->DebugTextOut(pos_info, u->pos, Colors::Green);
			//}
		}
	}
	void one_frame_bot::display_units_move_action(DebugInterface * debug, const Units & us) {
		for (auto u : us) {
			if (!(u->orders.empty()) && u->orders[0].ability_id == ABILITY_ID::MOVE) {
				//todo there must be something to do
			}
		}
	}
	void one_frame_bot::display_units_attack_action(DebugInterface * debug, const Units & us) {
		for (const Unit* u : us) {
			if (u->is_selected) {
				std::string attack_info;
				attack_info = std::to_string(u->weapon_cooldown);
				debug->DebugTextOut(attack_info, u->pos, Colors::Red);
				std::cout << attack_info << std::endl;
			}
		}
	}
	void one_frame_bot::display_movement(DebugInterface * debug, const Units & us) {
		for (const Unit* u : us) {
			if (u->is_selected) {
				std::cout << u->pos.x << '\t' << u->pos.y << std::endl;
			}
		}
	}
	void one_frame_bot::OnGameStart() {
		// initialize some data
		m_alive_self_units = Observation()->GetUnits(Unit::Alliance::Self);
		m_alive_enemy_units = Observation()->GetUnits(Unit::Alliance::Enemy);
		m_all_alive_units = Observation()->GetUnits();
		m_unit_types = Observation()->GetUnitTypeData();
		m_marine = m_unit_types[static_cast<int>(UNIT_TYPEID::TERRAN_MARINE)]; //? useless

	}
	void one_frame_bot::OnStep() {
		// 更新游戏数据
		m_alive_self_units = Observation()->GetUnits(Unit::Alliance::Self);
		m_alive_enemy_units = Observation()->GetUnits(Unit::Alliance::Enemy);
		m_all_alive_units = Observation()->GetUnits();
		m_game_info = Observation()->GetGameInfo();

		//! Debug部分
		// 输出线框
		DebugInterface* debug = Debug();
		display_fire_range(debug, m_all_alive_units);
		display_units_collision_range(debug, m_all_alive_units);
		//display_units_pos(debug, m_all_alive_units);
		//display_movement(debug, m_alive_self_units);
		//display_solution_line(m_selected_solution);
		display_units_attack_action(debug, m_alive_self_units);
		// until this step, those orders to debug are sent
		debug->SendDebug();

		solution s = run();
		deploy_solution(s);
	}
	void one_frame_bot::OnUnitIdle(const Unit * u) {
	}
	solution one_frame_bot::generate_random_solution() {
		solution so(m_alive_self_units.size());
		size_t alive_allies_number = m_alive_self_units.size();
		for (size_t i = 0; i < alive_allies_number; i++) {
			ActionRaw action;
			const Unit* unit = m_alive_self_units[i];
			// if this unit's weapon has been cooldown, randomly select to move or attack
			if (unit->weapon_cooldown <= 0.f && GetRandomFraction() < m_attack_prob) {
				// choose to attack
				Units targets = serach_units_can_be_attacked_by_unit(unit, Unit::Alliance::Enemy);
				// attack a unit within range
				if (targets.empty()) {
					action.ability_id = ABILITY_ID::MOVE;
					action.target_type = ActionRaw::TargetType::TargetPosition;
					Point2D new_pos = search_nearest_unit_from_point(unit->pos, Unit::Alliance::Enemy)->pos;
					new_pos = calculate_pos_next_frame(unit, new_pos);
					action.target_point = new_pos;
				}
				else {
					action.ability_id = ABILITY_ID::MOVE;
					action.target_type = ActionRaw::TargetType::TargetUnitTag;
					action.target_tag = GetRandomEntry(targets)->tag;
				}
			}
			// choose to move
			else {
				action.ability_id = ABILITY_ID::MOVE;
				action.target_type = ActionRaw::TargetPosition;
				action.target_point = unit->pos + generate_random_point_within_radius(basic_movement_one_frame(m_unit_types[unit->unit_type]));
			}
			std::get<0>(so[i]) = unit->tag;
			std::get<1>(so[i]) = { action };
		}
		return so;
	}
	void one_frame_bot::generate_random_solutions(population & pop, size_t size) {
		for (size_t i = 0; i < size; i++) {
			pop[i] = generate_random_solution();
		}
	}
	void one_frame_bot::generate_offspring(const population & parents, population & offspring, int spring_size) {
		offspring.clear();
		if (spring_size > m_population_size) {
			throw("I can't do it that using smaller parent population to generate larger child population, at least in this version@one_frame_bot::generate_offspring");
		}
		offspring.reserve(spring_size);
		for (size_t i = 0; i < parents.size() && i < spring_size; i += 2) {
			std::vector<solution> instant_children = produce(parents[i], parents[i + 1]);
			offspring.insert(offspring.begin() + i, instant_children.begin(), instant_children.end());
		}
	}
	std::vector<solution> one_frame_bot::produce(const solution & a, const solution & b) {
		std::vector<solution> children = cross_over(a, b);
		for (solution& c : children) {
			if (GetRandomFraction() < m_muatation_rate) {
				mutate(c);
			}
		}
		return children;
	}


	std::vector<solution> one_frame_bot::cross_over(const solution & a, const solution & b) {
		// randomly select two points and change all the commands between them
		std::vector<solution> offspring = { a,b };
		size_t start = GetRandomInteger(0, a.size() - 2);
		size_t end = GetRandomInteger(start, a.size() - 1);
		// exchange the segment between the two points
		for (int i = start; i <= end; i++) {
			swap(offspring[0][i], offspring[1][i]);
		}
		return offspring;
	}
	void one_frame_bot::mutate(solution & s) {
		// select a random command and mutate it
		command& cmd = GetRandomEntry(s);
		ActionRaw& action = get_action(cmd, 0);
		switch (static_cast<ABILITY_ID>(action.ability_id)) {
		case ABILITY_ID::MOVE: {
			// change the target point, use polar coordinates
			//todo there is no change in ability
			Point2DInPolar p_polar(action.target_point);
			p_polar.theta += m_theta_mutate_step; //todo there must be another change which is the step change.
			action.target_point = p_polar.toPoint2D();
			break;
		}
		case ABILITY_ID::ATTACK: {
			// change the target unit
			const Unit* e_u = get_execution_unit(cmd);
			Units us = serach_units_can_be_attacked_by_unit(get_execution_unit(cmd), Unit::Alliance::Enemy);
			action.target_tag = GetRandomEntry(us)->tag;
			break;
		}
		default:
			break;
		}
	}

	solution one_frame_bot::run() {
		m_population.resize(m_population_size);
		m_damage_objective.resize(m_population_size);
		m_threat_objectvie.resize(m_population_size);
		// 生成随机解
		generate_random_solutions(m_population, m_population_size);
		// 评估所有解
		evaluate_all_solutions(m_population, m_damage_objective, m_threat_objectvie);
		// 对解进行排序
		sort_solutions(m_population, m_damage_objective, m_threat_objectvie);
		// 循环演化
		for (size_t i = 0; i < m_produce_times; i++) {
			generate_offspring(m_population, m_offspring, m_offspring_size);
			m_population.insert(m_population.begin(), m_offspring.begin(), m_offspring.end());
			evaluate_all_solutions(m_population, m_damage_objective, m_threat_objectvie);
			sort_solutions(m_population, m_damage_objective, m_threat_objectvie);

			m_population.erase(m_population.begin() + m_population_size, m_population.end());
			m_threat_objectvie.erase(m_threat_objectvie.begin() + m_population_size, m_threat_objectvie.end());
			m_damage_objective.erase(m_damage_objective.begin() + m_population_size, m_damage_objective.end());
		}
		// 返回最终solution
		return m_population[0];
	}

	void one_frame_bot::evaluate_all_solutions(const population & p, std::vector<float> & total_damage, std::vector<float> & total_theft) {
		total_damage.resize(p.size());
		total_theft.resize(p.size());
		std::vector<std::thread> threads;
		for (size_t i = 0; i < p.size(); i++) {
			//? caution: the i must be copied rather than refered
			threads.push_back(std::thread{ [&,i]() {
				total_damage[i] = evaluate_single_solution_damage_next_frame(p[i]);
				total_theft[i] = evaluate_single_solution_theft_next_frame(p[i]);
			} });
			//total_damage[i] = evaluate_single_solution_damage_next_frame(m_population[i]);
			//total_theft[i] = evaluate_single_solution_theft_next_frame(m_population[i]);

		}
		for (size_t i = 0; i < threads.size(); i++) {
			threads[i].join();
		}
		return;
	}
	void one_frame_bot::sort_solutions(population & p, std::vector<float> & total_damage, std::vector<float> & total_theft) {
		// 对两个目标值逐个相减
		std::vector<float> final_objective(total_damage.size());
		std::transform(std::begin(total_damage), std::end(total_damage), std::begin(total_theft), std::begin(final_objective), std::minus<float>());
		//todo 此处需要优化
		std::vector<std::pair<int, float>> index_to_obj;
		for (size_t i = 0; i < final_objective.size(); i++) {
			index_to_obj.push_back(std::pair<int, float>(i, final_objective[i]));
		}
		std::sort(std::begin(index_to_obj), std::end(index_to_obj), [](const std::pair<int, float> a, const std::pair<int, float> b)->bool {return a.second > b.second; });
		// 通过上面计算的序进行交换
		population p_(p);
		std::vector<float> td_(total_damage);
		std::vector<float> th_(total_theft);
		for (size_t i = 0; i < p.size(); i++) {
			int index = index_to_obj[i].first;
			p[i] = p_[index];
			total_damage[i] = td_[index];
			total_theft[i] = th_[index];
		}
	}
	solution one_frame_bot::select_one_solution(const population & p, std::vector<float> & d, std::vector<float> & h) {
		float highest_fitness = m_damage_objective[0] - m_threat_objectvie[0];
		solution selected_solution = m_population[0];
		size_t index(0);
		for (size_t i = 0; i < m_population.size(); i++) {
			if (m_damage_objective[i] - m_threat_objectvie[i] > highest_fitness) {
				selected_solution = m_population[i];
				index = i;
			}
		}
		Debug()->DebugTextOut("obj1: " + std::to_string(m_damage_objective[index]) + "\t");
		Debug()->DebugTextOut("obj2: " + std::to_string(m_threat_objectvie[index]));
		Debug()->SendDebug();
		std::cout << "obj1:" << m_damage_objective[index] << "\t" << "obj2:" << m_threat_objectvie[index] << std::endl;
		return selected_solution;
	}
	void one_frame_bot::deploy_solution(const solution & s) {
		for (const command& c : s) {
			const Unit* execute_unit = Observation()->GetUnit(std::get<0>(c));
			const ActionRaw action = std::get<1>(c)[0];
			//todo check the target type
			switch (action.target_type) {
			case ActionRaw::TargetType::TargetNone:
				Actions()->UnitCommand(execute_unit, action.ability_id);
				break;
			case ActionRaw::TargetType::TargetPosition:
				Actions()->UnitCommand(execute_unit, action.ability_id, action.target_point);
				break;
			case ActionRaw::TargetType::TargetUnitTag:
				Actions()->UnitCommand(execute_unit, action.ability_id, action.target_tag);
				break;
			default:
				break;
			}
		}
	}
	//todo ensure this function thread safe 
	float one_frame_bot::evaluate_single_solution_damage_next_frame(const solution & s) {
		float total_damage = 0.0f;
		// for each unit in a solution
		for (const auto& c : s) {
			const Unit* u_c = get_execution_unit(c);
			if (!get_actions(c).empty()) {
				const ActionRaw action = get_action(c, 0);
				switch (static_cast<ABILITY_ID>(action.ability_id)) {
				case ABILITY_ID::ATTACK:
					switch (action.target_type) {
					case ActionRaw::TargetType::TargetUnitTag: {
						total_damage += damage_unit_to_unit(u_c, Observation()->GetUnit(get_action(c, 0).TargetUnitTag));
						break;
					}
					case ActionRaw::TargetType::TargetPosition: {
						const Unit* target = search_nearest_unit_from_point(action.target_point, Unit::Alliance::Enemy);
						total_damage += damage_unit_to_unit(u_c, target);
						break;
					}
					case ActionRaw::TargetType::TargetNone:
					default:
						//todo I haven't came up with a way to handle it, maybe the priciple is to attack the nearest enemy unit
						break;
					}
					break;
				default:
					break;
				}
			}
		}

		return total_damage;
	}
	float one_frame_bot::evaluate_single_solution_hurt_next_frame(const solution & s) {
		float total_hurt = 0.0f;
		Units enemy_units = Observation()->GetUnits(Unit::Alliance::Enemy);
		for (const Unit* u : enemy_units) {
			Weapon u_weapon = Observation()->GetUnitTypeData()[u->unit_type].weapons[0];
			float search_range = u_weapon.range + u->radius;
			Units possible_targets = search_units_can_be_attacked_by_unit_in_solution(u, s);
			const Unit* target(nullptr);
			if (!possible_targets.empty()) {
				target = GetRandomEntry(possible_targets);
				total_hurt += damage_weapon_to_unit(u_weapon, target);
			}
		}
		return total_hurt;
	}
	float one_frame_bot::evaluate_single_solution_theft_next_frame(const solution & s) {
		float threat_sum = 0;
		for (const auto& c : s) {
			switch (static_cast<ABILITY_ID>(get_action(c, 0).ability_id)) {
			case ABILITY_ID::MOVE:
				threat_sum += threat_from_units_to_unit_new_pos(m_alive_enemy_units, get_execution_unit(c), get_action(c, 0).target_point);
				break;
			case ABILITY_ID::ATTACK:
				threat_sum += threat_from_units_to_unit(m_alive_enemy_units, get_execution_unit(c));
				break;
			default:
				throw("there is no brunch to handle this ability@one_frame_bot::evaluate_single_solution_theft_next_frame");
				break;
			}
		}
		return threat_sum;
	}
}