#include "debug_utility.h"

void sc2::display_fire_range(DebugInterface* debug, const Units& us, const UnitTypes& utd) {
	for (auto u : us) {
		std::vector<Weapon> weapons = utd[u->unit_type].weapons;
		Color sphere_color = Colors::Blue;
		if (u->alliance == Unit::Alliance::Self || u->alliance == Unit::Alliance::Ally) {
			sphere_color = Colors::Red;
		}
		for (auto& w : weapons) {
			debug->DebugSphereOut(u->pos, w.range, sphere_color);
		}
	}
}

void sc2::display_units_collision_range(DebugInterface* debug, const Units& us) {
	for (auto u : us) {
		debug->DebugSphereOut(u->pos, u->radius);
	}
}

void sc2::display_units_pos(DebugInterface* debug, const Units& us, bool need_select) {
	for (auto u : us) {
		if (!need_select || u->is_selected) {
			std::string pos_info;
			pos_info = std::to_string(u->pos.x) + ", " + std::to_string(u->pos.y) + "," + std::to_string(u->pos.z);
			debug->DebugTextOut(pos_info, u->pos, Colors::Green);
		}
	}
}

void sc2::display_units_attack_action(DebugInterface* debug, const Units& us, bool need_select) {
	for (const Unit* u : us) {
		if (!need_select || u->is_selected) {
			std::string attack_info;
			attack_info = std::to_string(u->weapon_cooldown);
			debug->DebugTextOut(attack_info, u->pos, Colors::Red);
		}
	}
}
