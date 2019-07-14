#include "simulator.h"
#include <sc2api/sc2_coordinator.h>
#include <sc2lib/sc2_utils.h>
#include <cassert>
#include <iostream>
#include <set>

using namespace sc2;

void Simulator::CopyAndSetState(const ObservationInterface* ob_source, DebugRenderer* debug_renderer) {
    m_save = SaveMultiPlayerGame(ob_source);
    m_relative_units = LoadMultiPlayerGame(m_save, m_executor, *this);
    //! for test
    //todo check the save and relative_units
    for (const auto& unit_state : m_save.unit_states) {
        int count = 0;
        if (m_relative_units.find(unit_state.unit_tag) == m_relative_units.end()) {
            count++;
        }
        if (count > 0) {
            std::cout << "relationship mistake: " << count << std::endl;
        }
    }
    //check the crush of unit relationship
    std::set<const Unit*> check_set;
    for (const auto& item : m_relative_units) {
        check_set.insert(item.second);
    }
    // if (check_set.size() != m_relative_units.size() || m_relative_units.size() != Observation()->GetUnits().size()) {
    //     std::cout << "mistake in copy: " << m_relative_units.size() - check_set.size() << std::endl;
    //     std::cout << "difference between save and current units: " << m_relative_units.size() - Observation()->GetUnits().size() << std::endl;
    // }
    if (!IsMultiPlayerGame()) {
        m_executor.Control()->Save();
    }
    if (debug_renderer) {
        // Maybe I only need to display the result
        debug_renderer->ClearRenderer();
        //! here must be somthing wrong!
        // debug_renderer->DrawOrders(m_commands, m_executor.Observation(), m_relative_units);
        debug_renderer->DrawObservation(m_executor.Observation());
        debug_renderer->Present();
    }
}

void Simulator::SetUnitsRelations(State state, Units us_copied) {
    // since state object is static once it is saved, so it just needs to get
    // the relations between units in state and units in simulator
    for (const UnitState& state_u : state.unit_states) {
        m_relative_units[state_u.unit_tag] =
            SelectNearestUnitFromPoint(state_u.pos, us_copied);
    }
}

void Simulator::SetStartPoint(const std::vector<Command>& commands,
                              const ObservationInterface* ob) {
    CopyAndSetState(ob);
    SetOrders(commands);
}

void Simulator::Run(int steps, DebugRenderer* debug_renderer) {
    if (debug_renderer) {
        const ObservationInterface* ob = GetObservations().front();
        for (size_t i = 0; i < (size_t)ceil(steps / GetStepSize()); i++) {
            Update();
            debug_renderer->ClearRenderer();
            debug_renderer->DrawOrders(m_commands, m_executor.Observation(), m_relative_units);
            debug_renderer->DrawObservation(ob);
            debug_renderer->Present();
        }
    } else {
        for (size_t i = 0; i < (size_t)ceil(steps / GetStepSize()); i++) {
            Update();
        }
    }
}

const ObservationInterface* Simulator::Observation() const {
    return m_executor.Observation();
}

DebugInterface* Simulator::Debug() {
    return m_executor.Debug();
}

ActionInterface* Simulator::Actions() {
    return m_executor.Actions();
}

float Simulator::GetTeamHealthLoss(Unit::Alliance alliance) const {
    uint32_t player_id = 0;
    switch (alliance) {
        case sc2::Unit::Self:
            player_id = Observation()->GetPlayerID();
            break;
        //case sc2::Unit::Ally:
        //? I have no idea on how to handle that
        //	break;
        case sc2::Unit::Neutral:
            player_id = 0;
            break;
        case sc2::Unit::Enemy:
            player_id = Observation()->GetPlayerID() == 1 ? 2 : 1;
            break;
        default:
            break;
    }
    float health_loss = 0.f;
    // use the data from m_save and current data to simply calculate the health loss
    for (const UnitState& state_u : m_save.unit_states) {
        if (state_u.player_id == player_id) {
            const Unit* u = m_relative_units.at(state_u.unit_tag);
            // check if the unit has been dead
            // because the API will keep the a unit's health its number the last time he lived if he has been dead now
            // I need calculate the shield at the same time
            if (u->is_alive) {
                health_loss += state_u.life - u->health + state_u.shields - u->shield;
            } else {
                health_loss += state_u.life + state_u.shields;
            }
        }
    }
    return health_loss;
}

void Simulator::Load() {
    if (IsMultiPlayerGame()) {
        LoadMultiPlayerGame(m_save, m_executor, *this);
        SetUnitsRelations(m_save, m_executor.Observation()->GetUnits());
    } else {
        m_executor.Control()->Load();
        // I don't have to rebuild the relationships, the tags will keep
        // unchanged after Save() and Load()
    }
}

void Simulator::SetOrders(const std::vector<Command>& commands, DebugRenderer* debug_renderer) {
    // Just simply press those actions in every unit
    //? Note that the orders can be stored into units or somewhere else is
    //limited in StarCraft II, I need to figure out it
    for (const Command& cmd : commands) {
        for (const ActionRaw& act : cmd.actions) {
            switch (act.target_type) {
                case ActionRaw::TargetType::TargetNone:
                    m_executor.Actions()->UnitCommand(
                        m_relative_units[cmd.unit_tag], act.ability_id, true);
                    break;
                case ActionRaw::TargetType::TargetPosition:
                    m_executor.Actions()->UnitCommand(
                        m_relative_units[cmd.unit_tag], act.ability_id,
                        act.target_point, true);
                    break;
                case ActionRaw::TargetType::TargetUnitTag:
                    m_executor.Actions()->UnitCommand(
                        m_relative_units[cmd.unit_tag], act.ability_id,
                        m_relative_units[act.target_tag], true);
                    break;
            }
        }
    }
    m_commands = commands;
    if (debug_renderer) {
        // Maybe I only need to display the result
        debug_renderer->ClearRenderer();
        //! here must be somthing wrong!
        debug_renderer->DrawOrders(m_commands, m_executor.Observation(), m_relative_units);
        debug_renderer->DrawObservation(m_executor.Observation());
        debug_renderer->Present();
    }
}

void Simulator::SetOpponent(Agent* agent) {
    SetParticipants({CreateParticipant(Race::Terran, &m_executor),
                     CreateParticipant(Race::Terran, agent)});
}

void Simulator::SetOpponent(Difficulty difficulty) {
    SetParticipants({CreateParticipant(Race::Terran, &m_executor),
                     CreateComputer(Race::Terran, difficulty)});
}