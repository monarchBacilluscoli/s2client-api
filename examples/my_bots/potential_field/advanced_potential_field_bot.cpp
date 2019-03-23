#include "advanced_potential_field_bot.h"
#include "../utilities/sc2utility.h"

namespace sc2 {
	Vector2D sc2::advanced_potential_field_bot::force_enemy_to_unit(const Unit* enemy, const Unit* unit) {
		Vector2D force = (unit->pos - enemy->pos) / Distance2D(enemy->pos, unit->pos); // a unit vector
		return force * (calculate_enemy_repulsion_value(enemy, unit) - calculate_enemy_attraction_value(enemy, unit));
	}

	Vector2D advanced_potential_field_bot::force_ally_to_unit(const Unit* ally, const Unit* unit) {
		Vector2D force = (unit->pos - ally->pos) / Distance2D(ally->pos, unit->pos); // a unit vector
		return force * (calculate_ally_repulsion_value(ally, unit) - calculate_ally_repulsion_value(ally, unit));
	}

	float advanced_potential_field_bot::calculate_enemy_attraction_value(const Unit* enemy, const Unit* unit) const {
		std::vector<Weapon> weapons = sc2utility::get_matched_weapons(unit, enemy, m_unit_types);
		Weapon weapon;
		if (weapons.empty()) {
			return 0.f;
		}
		//! just consider the first weapon.
		weapon = weapons[0];

		float u_damage = sc2utility::damage_unit_to_unit(unit, enemy, m_unit_types);
		float u_attack_interval = weapon.speed * sc2utility::frames_per_second; //todo ceil
		float u_cooldown_damage = u_damage * (u_attack_interval - unit->weapon_cooldown) / u_attack_interval;
		float e_fragile = enemy->shield_max + unit->health_max - unit->shield - unit->health;
		float u_raw_range = weapon.range+enemy->radius+unit->radius;
		
		float slope = u_cooldown_damage + e_fragile + u_damage; //? why should I use u_damage as one of these items?
		float far_away = Distance2D(unit->pos, enemy->pos) - u_raw_range * m_ea_range_factor;
		if (far_away < 0) {
			return 0.f;
		}
		return far_away * slope;
	}

	float advanced_potential_field_bot::calculate_enemy_repulsion_value(const Unit* enemy, const Unit* unit) const {
		// no need to consider distance, this is not used for real damage calculation
		std::vector<Weapon> weapons = sc2utility::get_matched_weapons(enemy, unit, m_unit_types);
		Weapon weapon;
		if (weapons.empty()) {
			return 0.f;
		}
		//! just consider the first weapon.
		weapon = weapons[0];

		float e_damage = sc2utility::damage_unit_to_unit(enemy, unit, m_unit_types);
		float e_attack_interval = weapon.speed * sc2utility::frames_per_second; //todo ceil
		float e_cooldown_damage = e_damage * (e_attack_interval - enemy->weapon_cooldown) / e_attack_interval;
		float u_fragile = unit->shield_max + unit->health_max - unit->shield - unit->health;

		float raw_range = weapon.range + m_unit_types[enemy->unit_type].movement_speed * m_step_size / sc2utility::frames_per_second + enemy->radius + unit->radius;

		float slope = e_damage + e_cooldown_damage + u_fragile;
		//if two units overlap
		float dis = Distance2D(enemy->pos, unit->pos);
		//todo this should be modified
		float closeness = (raw_range * m_er_range_factor) - dis;
		if (closeness > 0) {
			return slope * closeness;
		}
		else {
			// out of 
			return 0;
		}
	}

	float advanced_potential_field_bot::calculate_ally_attraction_value(const Unit* ally, const Unit* unit) const {
		//todo need to be modified
		float slope = unit_cost(unit)*m_fa_value_factor; //todo use DPS or equivelant damage to handle the value
		float affect_range = m_fa_range_factor * unit_cost(unit);
		float dis = Distance2D(ally->pos,unit->pos); //! sub ally's radius
		if (affect_range > dis) {
			return slope * dis;
		}
		else {
			return 0.f; //? should here be a base value?
		}
	}
	float advanced_potential_field_bot::unit_cost(const Unit* unit) const{
		UnitTypeData type = m_unit_types[unit->tag];
		return type.mineral_cost + type.vespene_cost;
	}
}

