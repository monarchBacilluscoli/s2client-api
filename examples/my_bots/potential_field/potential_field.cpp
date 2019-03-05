#include "potential_field.h"

namespace sc2 {
	Vector2D sc2::potential_field_bot::force_enemy_to_unit(const Unit* source, const Unit* target) {
		float force_value = 0.f;
		float distance = Distance2D(source->pos, target->pos);
		float zero_field_dis = calculate_enemy_zero_field_dis(source, target);
		if (zero_field_dis != 0.f) {
			force_value = distance / zero_field_dis; //todo times damage
			if (force_value > 1.f) {
				force_value = 0.f;
			}
			else {
				force_value = 1 - force_value;
			}
		}
		return force_value*(target->pos - source->pos)/distance;
	}

	Vector2D potential_field_bot::force_ally_to_unit(const Unit* source, const Unit* target) {
		float force_value = 0.f;
		float distance = Distance2D(source->pos, target->pos);
		float zero_field_dis = calculate_ally_zero_field_dis(source, target);
		if(zero_field_dis>distance){
			force_value = 1 - distance/zero_field_dis;
		}
		else {
			force_value = zero_field_dis-distance;
		}
		return force_value * (target->pos - source->pos) / distance;
	}

	Vector2D potential_field_bot::force_wall_to_unit(const Unit* target) {
		//todo calculate the force given by walls of the map
		float zero_field_dis = m_unit_types[target->unit_type].weapons[0].range;
		float force_value_lr = 0;
		float force_value_tb = 0;
		
		// force from walls on left and right
		if (target->pos.x - m_game_info.playable_min.x < zero_field_dis) { // left to right
			force_value_lr = (target->pos.x - m_game_info.playable_min.x) / zero_field_dis;
		}
		else if (m_game_info.playable_max.x - target->pos.x < zero_field_dis) {
			force_value_lr = (target->pos.x - m_game_info.playable_max.x) / zero_field_dis;
		}
		// force from walls on top and bottom
		if (target->pos.y - m_game_info.playable_min.y < zero_field_dis) { // bottom to top
			force_value_tb = (target->pos.y - m_game_info.playable_min.y) / zero_field_dis;
		}
		else if (m_game_info.playable_max.y - target->pos.y < zero_field_dis){
			force_value_tb = (target->pos.y - m_game_info.playable_max.y) / zero_field_dis;
		}
		
		return  m_wall_force_factor*Vector2D(force_value_lr, force_value_tb);
	}

	Vector2D potential_field_bot::force_to_unit(const Units& sources, const Unit* u) {
		Vector2D force = {0.f,0.f};
		for (const Unit* source : sources) {
			if (source->alliance == Unit::Alliance::Self) {
				force += force_ally_to_unit(source, u);
			}
			else {
				force += force_enemy_to_unit(source, u);
			}
		}
		force += force_wall_to_unit(u);
		return force;
	}

	float potential_field_bot::calculate_enemy_zero_field_dis(const Unit* source, const Unit* target) {
		float dis;
		float target_range = m_unit_types[target->unit_type].weapons[0].range;
		float source_range = m_unit_types[source->unit_type].weapons[0].range;
		if (source_range < target_range) {
			dis = 0.7f * target_range + 0.3f * source_range + source->radius;
		}
		/*
		* for run
		*/
		else {
			dis = source_range + source->radius + target->radius;
		}
		return dis;
	}

	float potential_field_bot::calculate_ally_zero_field_dis(const Unit* source, const Unit* target) {
		return m_unit_types[target->unit_type].weapons[0].range* m_zero_ally_field_ratio;
	}

	Units potential_field_bot::serach_enemies_can_be_attacked_by_unit(const Unit* u) {
		Units can_be_attacked;
		Weapon weapon = m_unit_types[u->unit_type].weapons[0];
		for (const Unit* eu:m_alive_enemy_units) {
			if (Distance2D(eu->pos, u->pos)<=weapon.range+u->radius+eu->radius) {
				can_be_attacked.push_back(eu);
			}
		}
		return can_be_attacked;
	}

	void potential_field_bot::OnGameStart() {
		// initialize some data
		m_alive_self_units = Observation()->GetUnits(Unit::Alliance::Self);
		m_alive_enemy_units = Observation()->GetUnits(Unit::Alliance::Enemy);
		m_all_alive_units = Observation()->GetUnits();
		m_unit_types = Observation()->GetUnitTypeData();

		m_game_info = Observation()->GetGameInfo();
	}

	void potential_field_bot::OnStep() {
		// update game data
		m_alive_self_units = Observation()->GetUnits(Unit::Alliance::Self);
		m_alive_enemy_units = Observation()->GetUnits(Unit::Alliance::Enemy);
		m_all_alive_units = Observation()->GetUnits();

		//display_facing_direction(m_alive_self_units, Debug());

		// check each unit's condition
		for (const Unit* su: m_alive_self_units) {
			/* if su can attack:
			 * 1. has been cooldown
			 * 2. there is at least one unit in his fire_range
			*/
			if ((su->weapon_cooldown <= 0.f) && !serach_enemies_can_be_attacked_by_unit(su).empty()) { //todo can be optimized
				const Unit* target_u = select_nearest_unit_from_point(su->pos, m_alive_enemy_units);
				Actions()->UnitCommand(su, ABILITY_ID::ATTACK, target_u);
			}
			else {
				Vector2D force = force_to_unit(m_alive_enemy_units, su);
				display_force_direction(su, force, Debug());
				if (force != Vector2D()) {
					Point2D new_pos = su->pos + force / (force.x * force.x + force.y * force.y); // unit vector
					Actions()->UnitCommand(su, ABILITY_ID::MOVE, new_pos);
				}
				else {
					const Unit* target_u = select_nearest_unit_from_point(su->pos, m_alive_enemy_units);
					Actions()->UnitCommand(su, ABILITY_ID::MOVE, target_u);
				}
			}
			if (su->is_selected) {
				display_field_boundary(su, m_alive_enemy_units, Debug());
			}
		}
		display_fire_range(m_all_alive_units, Debug());
		display_playable_area(m_game_info,Debug());
		Debug()->SendDebug();
	}

	const Unit* potential_field_bot::select_nearest_unit_from_point(const Point2D& p, const Units& us) {
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

	void potential_field_bot::display_facing_direction(const Units& us,DebugInterface* debug) {
		for (const Unit* u : us) {
			// Show the direction of the unit.
			sc2::Point3D p1; // Use this to show target distance.
			{
				const float length = 5.0f;
				sc2::Point3D p0 = u->pos;
				p0.z += 0.1f; // Raise the line off the ground a bit so it renders more clearly.
				p1 = u->pos;
				assert(u->facing >= 0.0f && u->facing < 6.29f);
				p1.x += length * std::cos(u->facing);
				p1.y += length * std::sin(u->facing);
				debug->DebugLineOut(p0, p1, sc2::Colors::Green);
			}
		}
	}

	void potential_field_bot::display_force_direction(const Unit* u, const Vector2D& force, DebugInterface* debug) {
		Vector3D force3D(force.x, force.y, 1.f);
		debug->DebugLineOut(u->pos, u->pos + force3D, Colors::Yellow);
	}

	void potential_field_bot::display_field_boundary(const Unit* selected_u, const Units& source_us, DebugInterface* debug) {
		for (const Unit* eu: source_us) {
			float field_radius = calculate_enemy_zero_field_dis(eu, selected_u);
			debug->DebugSphereOut(eu->pos, field_radius, Colors::Blue);
		}
	}

	void potential_field_bot::display_fire_range(const Units& us, DebugInterface* debug) {
		for (auto u : us) {
			std::vector<Weapon> weapons = Observation()->GetUnitTypeData()[u->unit_type].weapons;
			for (auto& w : weapons) {
				debug->DebugSphereOut(u->pos, w.range, Colors::Red);
			}
		}
	}

	void potential_field_bot::display_playable_area(const GameInfo& gi, DebugInterface* debug) {
		debug->DebugBoxOut(
			Point3D(gi.playable_min.x, gi.playable_min.y, gi.height),
			Point3D(gi.playable_max.x, gi.playable_max.y, gi.height)
			);
	}



}

