#include "simulator.h"
#include <sc2api/sc2_coordinator.h>
#include <sc2lib/sc2_utils.h>
#include <cassert>
#include <iostream>
#include <set>

using namespace sc2;

void Executor::OnStep()
{
    // on each step, check if each unit has finished its current command, if so, set the next order for it
    if (m_is_setting || m_commands.empty())
    {
        return;
    }
    // std::cout << m_commands.size() << "\t" << std::flush;
    Units us = Observation()->GetUnits(Unit::Alliance::Self);
    for (const Unit *u : us)
    {
        bool has_cooldown_time = (m_cooldown_last_frame.find(u->tag) != m_cooldown_last_frame.end());
        // check if the current has been finished
        if (u->orders.empty() ||                                                           // no order now
            (has_cooldown_time && (m_cooldown_last_frame[u->tag] < u->weapon_cooldown)) || // this unit has executed a new attack just now
            (!has_cooldown_time && u->weapon_cooldown > 0.f))                              // the first time this unit attack (I think it can not happen)
        {
            // execute the next action
            if (m_commands.find(u->tag) == m_commands.end())
            {
                // std::cout << m_commands.size() << "\t" << std::flush;
                std::cout << "mistake" << std::endl;
            }
            if (!m_commands.at(u->tag).empty())
            {
                ActionRaw action = m_commands.at(u->tag).front();
                //todo distinguish the attack action and move action
                if (action.ability_id == ABILITY_ID::ATTACK)
                {
                    switch (action.target_type)
                    {
                    case ActionRaw::TargetType::TargetNone:
                    {
                        Actions()->UnitCommand(u, action.ability_id);
                    }
                    break;
                    case ActionRaw::TargetType::TargetPosition:
                    {
                        //move threr then attack automatically according to the game AI
                        Actions()->UnitCommand(u, ABILITY_ID::MOVE, action.target_point);
                        Actions()->UnitCommand(u, action.ability_id, action.target_point, true);
                    }
                    break;
                    case ActionRaw::TargetType::TargetUnitTag:
                    {
                        // directly deploy
                        Actions()->UnitCommand(u, action.ability_id, action.target_tag);
                    }
                    default:
                        break;
                    }
                }
                else //! for now, "else" means move action
                {
                    switch (action.target_type)
                    {
                    case ActionRaw::TargetType::TargetNone:
                    {
                        Actions()->UnitCommand(u, action.ability_id);
                    }
                    break;
                    case ActionRaw::TargetType::TargetPosition:
                    {
                        Actions()->UnitCommand(u, action.ability_id, action.target_point);
                    }
                    break;
                    case ActionRaw::TargetType::TargetUnitTag:
                    {
                        Actions()->UnitCommand(u, action.ability_id, action.target_tag);
                    }
                    default:
                        break;
                    }
                }
                m_commands.at(u->tag).pop();
            }
            else
            {
                //todo if no actions available, what can I do?
            }
        }
        m_cooldown_last_frame[u->tag] = u->weapon_cooldown;
    }
}

// void Executor::OnUnitDestroyed(const Unit* u){
//     if(m_is_setting){
//         return;
//     }
//     //todo delete the commands and state
//     m_cooldown_last_frame.erase(u->tag);
//     m_commands.erase(u->tag);
// }

void Executor::SetCommands(const std::vector<Command> &commands)
{
    //todo copy the vector to queue member
    m_commands.clear();
    for (const Command &command : commands)
    {
        // if (!Observation()->GetUnit(command.unit_tag))
        // {
        //     std::cout << "no this unit here" << std::endl;
        // }
        std::queue<ActionRaw> &target_command = m_commands[command.unit_tag] = std::queue<ActionRaw>();
        for (const ActionRaw &action : command.actions)
        {
            // set the queue
            target_command.push(action);
        }
    }
    // if (m_commands.size() == 1)
    // {
    //     std::cout<<"m_commands.size()==1 @Executor::SetCommands()"<<std::flush;
    // }
}

void Executor::ClearCommands()
{
    m_commands.clear();
}

void Executor::SetIsSetting(bool is_setting)
{
    m_is_setting = is_setting;
}

void Executor::ClearCooldownData()
{
    m_cooldown_last_frame.clear();
}

void Simulator::CopyAndSetState(const ObservationInterface *ob_source, DebugRenderer *debug_renderer)
{
    m_executor.ClearCooldownData();
    m_executor.ClearCommands();
    m_executor.SetIsSetting(true);
    m_save = SaveMultiPlayerGame(ob_source);
    m_relative_units = LoadMultiPlayerGame(m_save, m_executor, *this);
    //! for test
    //todo check the save and relative_units
    // for (const auto& unit_state : m_save.unit_states) {
    //     int count = 0;
    //     if (m_relative_units.find(unit_state.unit_tag) == m_relative_units.end()) {
    //         count++;
    //     }
    //     if (count > 0) {
    //         std::cout << "relationship mistake: " << count << std::endl;
    //     }
    // }
    //! check the crush of unit relationship
    std::set<const Unit *> check_set;
    for (const auto &item : m_relative_units)
    {
        check_set.insert(item.second);
    }
    if (check_set.size() != m_relative_units.size() || m_relative_units.size() != Observation()->GetUnits().size())
    {
        std::cout << "mistake in copy: " << m_relative_units.size() - check_set.size() << std::endl;                                           // it means that some units were mistaken to be seen as the same one
        std::cout << "difference between save and current units: " << m_relative_units.size() - Observation()->GetUnits().size() << std::endl; // it can mean that some units has been dead
    }
    if (!IsMultiPlayerGame())
    {
        m_executor.Control()->Save();
    }
    if (debug_renderer)
    {
        // Maybe I only need to display the result
        debug_renderer->ClearRenderer();
        //! here must be somthing wrong!
        debug_renderer->DrawObservation(m_executor.Observation());
        debug_renderer->Present();
    }
    m_executor.SetIsSetting(false);
}

void Simulator::SetUnitsRelations(State state, Units us_copied)
{
    // since state object is static once it is saved, so it just needs to get
    // the relations between units in state and units in simulator
    for (const UnitState &state_u : state.unit_states)
    {
        m_relative_units[state_u.unit_tag] =
            SelectNearestUnitFromPoint(state_u.pos, us_copied);
    }
}

void Simulator::SetStartPoint(const std::vector<Command> &commands,
                              const ObservationInterface *ob)
{
    CopyAndSetState(ob);
    SetOrders(commands);
}

void Simulator::Run(int steps, DebugRenderer *debug_renderer)
{
    // set false to let simu bot to use the normal mode to call the OnStep
    m_executor.SetIsSetting(false);
    int game_loop = (size_t)ceil(steps / GetStepSize());
    if (debug_renderer)
    {
        const ObservationInterface *ob = GetObservations().front();
        for (size_t i = 0; i < game_loop; i++)
        {
            Update();
            debug_renderer->ClearRenderer();
            debug_renderer->DrawOrders(m_commands, m_executor.Observation());
            debug_renderer->DrawObservation(ob);
            debug_renderer->Present();
        }
    }
    else
    {
        for (size_t i = 0; i < game_loop; i++)
        {
            Update();
        }
    }
    m_executor.SetIsSetting(true);
}

const ObservationInterface *Simulator::Observation() const
{
    return m_executor.Observation();
}

DebugInterface *Simulator::Debug()
{
    return m_executor.Debug();
}

ActionInterface *Simulator::Actions()
{
    return m_executor.Actions();
}

float Simulator::GetTeamHealthLoss(Unit::Alliance alliance) const
{
    uint32_t player_id = 0;
    switch (alliance)
    {
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
    for (const UnitState &state_u : m_save.unit_states)
    {
        if (state_u.player_id == player_id)
        {
            const Unit *u = m_relative_units.at(state_u.unit_tag);
            // check if the unit has been dead
            // because the API will keep the a unit's health its number the last time he lived if he has been dead now
            // I need calculate the shield at the same time
            if (u->is_alive)
            {
                health_loss += state_u.life - u->health + state_u.shields - u->shield;
            }
            else
            {
                health_loss += state_u.life + state_u.shields;
            }
        }
    }
    return health_loss;
}

void Simulator::Load()
{
    if (IsMultiPlayerGame())
    {
        LoadMultiPlayerGame(m_save, m_executor, *this);
        SetUnitsRelations(m_save, m_executor.Observation()->GetUnits());
    }
    else
    {
        m_executor.Control()->Load();
        // I don't have to rebuild the relationships, the tags will keep
        // unchanged after Save() and Load()
    }
}

void Simulator::SetOrders(const std::vector<Command> &commands, DebugRenderer *debug_renderer)
{
    // Just simply press those actions in every unit
    //? Note that the orders can be stored into units or somewhere else is limited in StarCraft II, I need to figure out it
    m_original_commands = commands;
    m_commands = commands;
    m_executor.SetIsSetting(true);
    // std::cout << "original commands size: " << commands.size()<<std::endl;
    // translate the command into local tag command
    for (Command &cmd : m_commands)
    {
        cmd.unit_tag = m_relative_units.at(cmd.unit_tag)->tag;
        for (ActionRaw &act : cmd.actions)
        {
            if (act.target_type == ActionRaw::TargetUnitTag)
            {
                act.target_tag = m_relative_units.at(cmd.unit_tag)->tag;
            }
        }
    }
    m_executor.SetCommands(m_commands);
    //todo need to translate the command, I mean the tags of units from the original process to simulation process
    if (debug_renderer)
    {
        // Maybe I only need to display the result
        debug_renderer->ClearRenderer();
        debug_renderer->DrawOrders(m_commands, m_executor.Observation());
        debug_renderer->DrawObservation(m_executor.Observation());
        debug_renderer->Present();
    }
    m_executor.SetIsSetting(false);
}

void Simulator::SetDirectOrders(const std::vector<Command> &commands, DebugRenderer *debug_renderer)
{
    m_commands = commands;
    m_executor.SetIsSetting(true);
    m_executor.SetCommands(m_commands);
    if (debug_renderer)
    {
        // Maybe I only need to display the result
        debug_renderer->ClearRenderer();
        debug_renderer->DrawOrders(m_commands, m_executor.Observation());
        debug_renderer->DrawObservation(m_executor.Observation());
        debug_renderer->Present();
    }
    m_executor.SetIsSetting(false);
}

void Simulator::SetOpponent(Agent *agent)
{
    SetParticipants({CreateParticipant(Race::Terran, &m_executor),
                     CreateParticipant(Race::Terran, agent)});
}

void Simulator::SetOpponent(Difficulty difficulty)
{
    SetParticipants({CreateParticipant(Race::Terran, &m_executor),
                     CreateComputer(Race::Terran, difficulty)});
}