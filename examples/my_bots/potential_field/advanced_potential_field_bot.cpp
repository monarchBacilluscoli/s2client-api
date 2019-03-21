#include "advanced_potential_field_bot.h"

float sc2::advanced_potential_field_bot::calculate_enemy_repulsion_value(const Unit* enemy, const Unit* unit) const {
	float force_value = 0.f;
	float distance = Distance2D(enemy->pos, unit->pos);
	float zero_field_dis = calculate_enemy_zero_field_dis(enemy, unit);
	if (zero_field_dis != 0.f) {
		force_value = distance / zero_field_dis; //todo times damage
		if (force_value > 1.f) {
			force_value = 0.f;
		}
		else {
			force_value = 1 - force_value;
		}
	}
	return force_value;
}
