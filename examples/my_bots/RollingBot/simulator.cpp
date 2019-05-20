#include "simulator.h"
#include <cassert>
#include <sc2api/sc2_coordinator.h>
#include"../utilities/sc2utility.h"
using namespace sc2;


void Simulator::CopyAndSetState(const ObservationInterface* ob_source)
{
	// save from local instance
	m_save = SaveMultiPlayerGame(ob_source); 
	LoadMultiPlayerGame(m_save, m_executor, m_coordinator);
	SetUnitsRelations(m_save, m_executor.Observation()->GetUnits());
	if (!m_is_multi_player) {
		m_executor.Control()->Save();
	}
}

void sc2::Simulator::SetUnitsRelations(State state, Units us_copied)
{
	// since state object is static once it is saved, so it just needs to get the relations between units in state and units in simulator
	for (const UnitState state_u : state.unit_states)
	{
		m_relative_units[state_u.unit_tag] = sc2utility::select_nearest_unit_from_point(state_u.pos, us_copied);
	}
}

void sc2::Simulator::SetStartPoint(std::vector<command> commands, const ObservationInterface* ob)
{
	CopyAndSetState(ob);
	SetOrders(commands);
}

void sc2::Simulator::Run(int steps)
{	
	for (size_t i = 0; i < steps; i++)
	{
		m_coordinator.Update();
	}
}

const ObservationInterface* sc2::Simulator::Observation() const
{
	return m_executor.Observation();
}

void sc2::Simulator::Load()
{
	if (m_is_multi_player) {
		LoadMultiPlayerGame(m_save, m_executor, m_coordinator);
		SetUnitsRelations(m_save, m_executor.Observation()->GetUnits());
	}
	else {
		m_executor.Control()->Load();
		//todo I need to figure out whether or not the save & load don't reset the tags of units
	}
}

void sc2::Simulator::SetOrders(std::vector<command> commands)
{
	// Just simply press those actions in every unit
	//? Note that the orders can be stored into units or somewhere else is limited in StarCraft II, I need to figure out it
	for (const command& cmd:commands)
	{
		for (const ActionRaw& act:cmd.actions)
		{
			m_executor.Actions()->UnitCommand(m_relative_units[cmd.unit_tag], act.ability_id, true);
		}
	}
}

void sc2::Simulator::Initialize(std::string net_address, int port_start, std::string map_path, int step_size, Agent& my_bot, PlayerSetup opponent, bool Multithreaded)
{
	//this simulator don't support observer type player
	assert(opponent.type == PlayerType::Computer || opponent.type == PlayerType::Participant);
	//sets realtime false
	m_coordinator.SetRealtime(false);
	//sets Multithreaded
	m_coordinator.SetMultithreaded(Multithreaded);
	m_coordinator.SetStepSize(step_size);
	m_is_multi_player = opponent.type == PlayerType::Participant;
	//sets the bot which can deploy my orders, as for the other one...
	m_executor = my_bot;
	m_coordinator.SetParticipants({
		CreateParticipant(Race::Terran, &m_executor),
		opponent
		});
	m_coordinator.SetNetAddress(net_address);

	//only when the participant is a bot, SetupPorts shuld be called to set all the ports to be used.
	if (opponent.type == PlayerType::Participant) {
		m_coordinator.SetupPorts(2, port_start + 1);
	}

	m_coordinator.Connect(port_start);
	m_coordinator.StartGame(map_path);
	//? should I kill all the unit for a clean map?
}

//! it should be called before initialization
void sc2::Simulator::SetFeatureLayers(const FeatureLayerSettings& settings)
{
	m_coordinator.SetFeatureLayers(settings);
}
