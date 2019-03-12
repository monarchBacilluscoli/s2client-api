#include"attack_nearest.h"

namespace sc2 {
	void attack_nearest::OnUnitIdle(const Unit* unit) {
		const Unit* target = select_nearest_unit_from_point(unit->pos, m_alive_enemy_units);
		Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, target);
	}
	void attack_nearest::OnGameStart() {
		// initialize some data
		m_alive_self_units = Observation()->GetUnits(Unit::Alliance::Self);
		m_alive_enemy_units = Observation()->GetUnits(Unit::Alliance::Enemy);
		m_all_alive_units = Observation()->GetUnits();
		m_unit_types = Observation()->GetUnitTypeData();

		m_game_info = Observation()->GetGameInfo();
	}
	void attack_nearest::OnStep() {
		// update game data
		m_alive_self_units = Observation()->GetUnits(Unit::Alliance::Self);
		m_alive_enemy_units = Observation()->GetUnits(Unit::Alliance::Enemy);
		m_all_alive_units = Observation()->GetUnits();
	}
}