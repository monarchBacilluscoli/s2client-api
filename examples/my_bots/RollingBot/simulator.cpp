#include "simulator.h"
#include <cassert>
#include <sc2api/sc2_coordinator.h>
#include"../utilities/sc2utility.h"
using namespace sc2;


void Simulator::CopyAndSetState(const ObservationInterface* ob_source)
{
	// save from local instance
	m_save = SaveMultiPlayerGame(ob_source); 
	LoadMultiPlayerGame(m_save, m_executor, *this);
	SetUnitsRelations(m_save, m_executor.Observation()->GetUnits());
	if (!IsMultiPlayerGame()) {
		m_executor.Control()->Save();
	}
}

void Simulator::SetUnitsRelations(State state, Units us_copied)
{
	// since state object is static once it is saved, so it just needs to get the relations between units in state and units in simulator
	for (const UnitState& state_u : state.unit_states)
	{
		m_relative_units[state_u.unit_tag] = sc2utility::select_nearest_unit_from_point(state_u.pos, us_copied);
	}
}

void Simulator::SetStartPoint(std::vector<Command> commands, const ObservationInterface* ob)
{
	CopyAndSetState(ob);
	SetOrders(commands);
}

void Simulator::Run(int steps)
{	
	for (size_t i = 0; i < steps; i++)
	{
		Update();
	}
}

const ObservationInterface* Simulator::Observation() const
{
	return m_executor.Observation();
}

float Simulator::GetTeamHealthLoss(Unit::Alliance alliance) const {
	float health_loss = 0.f;
	// use the data from m_save and current data to simply calculate the health loss
	for (const UnitState& state_u : m_save.unit_states)
	{
		const Unit* u = m_relative_units.at(state_u.unit_tag);
		Units us = m_executor.Observation()->GetUnits();
		// check if the unit has been dead
		// because the API will keep the a unit's health its number the last time he lived if he has been dead now
		if (u->is_alive) {
			health_loss += state_u.life - u->health;
		}
		else {
			health_loss += state_u.life;
		}
	}
	return health_loss;
}

void Simulator::Load()
{
	if (IsMultiPlayerGame()) {
		LoadMultiPlayerGame(m_save, m_executor, *this);
		SetUnitsRelations(m_save, m_executor.Observation()->GetUnits());
	}
	else {
		m_executor.Control()->Load();
		// I don't have to rebuild the relationships, the tags will keep unchanged after Save() and Load()
	}
}

void Simulator::SetOrders(std::vector<Command> commands)
{
	// Just simply press those actions in every unit
	//? Note that the orders can be stored into units or somewhere else is limited in StarCraft II, I need to figure out it
	for (const Command& cmd:commands)
	{
		for (const ActionRaw& act:cmd.actions)
		{
			switch (act.target_type)
			{
			case ActionRaw::TargetType::TargetNone:
				m_executor.Actions()->UnitCommand(m_relative_units[cmd.unit_tag], act.ability_id, true);
				break;
			case ActionRaw::TargetType::TargetPosition:
				m_executor.Actions()->UnitCommand(m_relative_units[cmd.unit_tag], act.ability_id, act.target_point, true);
				break;
			case ActionRaw::TargetType::TargetUnitTag:
				m_executor.Actions()->UnitCommand(m_relative_units[cmd.unit_tag], act.ability_id, m_relative_units[act.target_tag], true);
				break;
			}
		}
	}
}

void Simulator::SetOpponent(Agent* agent)
{
	SetParticipants({
	CreateParticipant(Race::Terran, &m_executor),
	CreateParticipant(Race::Terran, agent)
		});
}

void Simulator::SetOpponent(Difficulty difficulty)
{
	SetParticipants({
	CreateParticipant(Race::Terran, &m_executor),
	CreateComputer(Race::Terran, difficulty)
		});
}