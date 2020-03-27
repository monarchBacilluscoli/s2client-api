#include "simulator.h"
#include <sc2api/sc2_coordinator.h>
#include <sc2lib/sc2_utils.h>
#include <cassert>
#include <iostream>
#include <set>

using namespace sc2;

void Simulator::CopyAndSetState(const ObservationInterface *ob_source
#ifdef USE_GRAPHICS
                                ,
                                DebugRenderer *debug_renderer
#endif //USE_GRAPHICS
)
{
    m_executor.Clear();
    m_executor.SetIsSetting(true);
    m_save = SaveMultiPlayerGame(ob_source);
    m_relative_units = LoadMultiPlayerGame(m_save, m_executor, *this);
    SetReversedUnitRelation(m_target_to_source_unit_tags, m_relative_units);
    m_executor.Initialize();
    { //! check the crush of unit relationship
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
    }
    if (!IsMultiPlayerGame())
    {
        m_executor.Control()->Save();
    }
#ifdef USE_GRAPHICS
    if (debug_renderer)
    {
        // Maybe I only need to display the result
        debug_renderer->ClearRenderer();
        //! here must be somthing wrong!
        debug_renderer->DrawObservation(m_executor.Observation());
        debug_renderer->Present();
    }
#endif // USE_GRAPHICS
    m_executor.SetIsSetting(false);
}

std::thread::id Simulator::Run(int steps
#ifdef USE_GRAPHICS
                               ,
                               DebugRenderer *debug_renderer
#endif // USE_GRAPHICS
)
{
    // set false to let simu bot to use the normal mode to call the OnStep
    m_executor.SetIsSetting(false);
    int game_loop = (size_t)ceil(steps / GetStepSize());
#ifdef USE_GRAPHICS
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
#else
    for (size_t i = 0; i < game_loop; i++)
    {
        Update();
    }
#endif // USE_GRAPHICS
    m_executor.SetIsSetting(true);
    return std::this_thread::get_id();
}

void Simulator::SetOrders(const std::vector<Command> &commands
#ifdef USE_GRAPHICS
                          ,
                          DebugRenderer *debug_renderer
#endif // USE_GRAPHICS
)
{
    // Just simply press those actions into every unit
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
#ifdef USE_GRAPHICS
    if (debug_renderer)
    {
        // Maybe I only need to display the result
        debug_renderer->ClearRenderer();
        debug_renderer->DrawOrders(m_commands, m_executor.Observation());
        debug_renderer->DrawObservation(m_executor.Observation());
        debug_renderer->Present();
    }
#endif // USE_GRAPHICS
    m_executor.SetIsSetting(false);
}

void Simulator::SetDirectOrders(const std::vector<Command> &commands
#ifdef USE_GRAPHICS
                                ,
                                DebugRenderer *debug_renderer
#endif // USE_GRAPHICS
)
{
    m_commands = commands;
    m_executor.SetIsSetting(true);
    m_executor.SetCommands(m_commands);
#ifdef USE_GRAPHICS
    if (debug_renderer)
    {
        // Maybe I only need to display the result
        debug_renderer->ClearRenderer();
        debug_renderer->DrawOrders(m_commands, m_executor.Observation());
        debug_renderer->DrawObservation(m_executor.Observation());
        debug_renderer->Present();
    }
#endif // USE_GRAPHICS
    m_executor.SetIsSetting(false);
}

void Simulator::SetUnitsRelations(State state, Units us_copied)
{
    // since state object is static once it is saved, so it just needs to get
    // the relations between units in state and units in simulator
    for (const UnitState &state_u : state.unit_states)
    {
        m_relative_units[state_u.unit_tag] =
            FindNearestUnitFromPoint(state_u.pos, us_copied);
    }
}

void Simulator::SetStartPoint(const std::vector<Command> &commands,
                              const ObservationInterface *ob)
{
    CopyAndSetState(ob);
    SetOrders(commands);
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

ControlInterface *Simulator::Control()
{
    return m_executor.Control();
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
                health_loss += state_u.life - u->health + state_u.shield - u->shield;
            }
            else
            {
                health_loss += state_u.life + state_u.shield;
            }
        }
    }
    return health_loss;
}

const std::list<Unit> &Simulator::GetTeamDeadUnits(Unit::Alliance alliance) const
{
    return m_executor.GetDeadUnits(alliance);
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

std::string Simulator::GenerateSimMapPath(const std::string &map_path)
{
    if (map_path.rfind("Sim.SC2Map") != std::string::npos)
    {
        return map_path;
    }
    std::string sim_map_path = map_path;
    size_t insert_index = sim_map_path.rfind(".SC2Map");
    sim_map_path.insert(std::min(sim_map_path.size(), insert_index), "Sim");
    return sim_map_path;
}

void Simulator::SetReversedUnitRelation(std::map<Tag, Tag> &target_to_source_units, const std::map<Tag, const Unit *> &relative_units)
{
    target_to_source_units.clear();
    for (const auto &pair : relative_units)
    {
        target_to_source_units[pair.second->tag] = pair.first;
    }
    if (target_to_source_units.size() != relative_units.size())
    {
        std::cerr << "copy mistake!@" << __FUNCTION__ << std::endl;
    }
    return;
}

std::map<Tag, UnitStatisticalData> Simulator::GetUnitsStatistics()
{
    std::map<Tag, UnitStatisticalData> units_statistics;
    const std::map<Tag, UnitStatisticalData> &raw_statistics = m_executor.GetUnitsStatistics();
    for (const auto &u : m_save.unit_states) // tag from main process
    {
        units_statistics[u.unit_tag] = raw_statistics.at(m_relative_units[u.unit_tag]->tag);
    }
    return units_statistics;
}

const UnitStatisticalData &Simulator::GetUnitStatistics(Tag tag)
{
    return m_executor.GetUnitStatistics(m_relative_units[tag]->tag);
}

GameResult Simulator::CheckGameResult() const
{
    return m_executor.CheckGameResult();
}