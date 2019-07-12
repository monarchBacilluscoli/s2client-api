#include "state.h"
#include "sc2lib/sc2_utils.h"
#include <sc2utils/sc2_manage_process.h>

using namespace sc2;

State sc2::SaveMultiPlayerGame(const ObservationInterface* observation)
{
	State save;
	Units us = observation->GetUnits();
	save.unit_states.resize(us.size());
	const Unit* u;
	for (size_t i = 0; i < us.size(); i++)
	{
		u = us.at(i);
		save.unit_states[i] = UnitState({
			u->unit_type,
			u->pos,
			static_cast<uint32_t>(u->owner),
			u->tag, // this is just used for remote simulation
			u->energy,
			u->health,
			u->shield
			});
	}
	return save;
}

std::map<Tag, const Unit*> sc2::LoadMultiPlayerGame(State save, Client& current_client, Coordinator& current_coordinator)
{
    std::map<Tag, const Unit*> unit_map;
    // kills all current units
    for (const Unit* u : current_client.Observation()->GetUnits()) {
        current_client.Debug()->DebugKillUnit(u);
	}
	current_client.Debug()->SendDebug();
	// the wrekages need about 20 game loops to be cleaned
	for (size_t i = 0; i < 20; i++)
	{
		current_coordinator.Update();
	}
	// creates units from save
	for (UnitState state_u: save.unit_states)
	{
		current_client.Debug()->DebugCreateUnit(state_u.unit_type, state_u.pos, state_u.player_id);
	}
	current_client.Debug()->SendDebug();
	// the DebugCreateUnit() needs at least 1 loops to be executed
	// current_coordinator.Update();
	current_coordinator.Update();
	// from here, I need to ensure nobody will move, but if I do it like this...x
	// todo set the relationship here! To ensure the minimize movement
	// just copys the units in save
	const Unit* u_copied;
	Units us_copied = current_client.Observation()->GetUnits();
	for (const UnitState& state_u : save.unit_states) {
		unit_map[state_u.unit_tag] = u_copied = SelectNearestUnitFromPoint(state_u.pos, us_copied);
		current_client.Debug()->DebugSetShields(state_u.shields + 0.1f, u_copied); // WTF, shield cannot be set to 0, if you set 0, you will find it is full in game, but if you set it as 0.1f, the data in game will be 0, fuck again.
		current_client.Debug()->DebugSetLife(state_u.life,u_copied);
		current_client.Debug()->DebugSetEnergy(state_u.energy, u_copied);
	}
	current_client.Debug()->SendDebug();
	// like CreateUnit(), DebugCreateUnit() needs at least 2 loops to be executed, too
	current_coordinator.Update();
	// current_coordinator.Update();
    return unit_map;
}