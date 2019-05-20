#include "state.h"
#include "../utilities/sc2utility.h"

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

void sc2::LoadMultiPlayerGame(State save, Client& client, Coordinator& coordinator)
{
	// kills all current units
	for (const Unit* u : client.Observation()->GetUnits())
	{
		client.Debug()->DebugKillUnit(u);
	}
	// creates units from save
	for (UnitState state_u: save.unit_states)
	{
		client.Debug()->DebugCreateUnit(state_u.unit_type, state_u.pos, state_u.player_id);
	}
	client.Debug()->SendDebug();
	coordinator.Update();

	// just copys the units in save
	const Unit* u_copied;
	Units us_copied = client.Observation()->GetUnits();
	for (UnitState state_u : save.unit_states) {
		u_copied = sc2utility::select_nearest_unit_from_point(state_u.pos, us_copied);
		client.Debug()->DebugSetLife(state_u.life,u_copied);
		client.Debug()->DebugSetEnergy(state_u.energy, u_copied);
		client.Debug()->DebugSetShields(state_u.energy, u_copied);
	}
	client.Debug()->SendDebug();
	coordinator.Update();
}
