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
    m_enemy_executor.Clear();
    m_executor.SetIsSetting(true);
    m_enemy_executor.SetIsSetting(true);
    m_save = SaveMultiPlayerGame(ob_source);
    m_relative_units = LoadMultiPlayerGame(m_save, m_executor, *this);
    SetReversedUnitRelation(m_target_to_source_unit_tags, m_relative_units);
    m_executor.Initialize();
    m_enemy_executor.Initialize();
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
    m_enemy_executor.SetIsSetting(false);
    return;
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
    m_enemy_executor.SetIsSetting(false);
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
    m_enemy_executor.SetIsSetting(true);
    return std::this_thread::get_id();
}

void Simulator::SetOrders(const std::vector<Command> &commands, const std::vector<Command> &enemy_commands
#ifdef USE_GRAPHICS
                          ,
                          DebugRenderer *debug_renderer
#endif // USE_GRAPHICS
)
{
    // Just simply press those actions into every unit
    //? Note that the orders can be stored into units or somewhere else is limited in StarCraft II, I need to figure out it
    m_original_commands = commands;
    m_original_enemy_commands = enemy_commands;
    m_commands = commands;
    m_enemy_commands = enemy_commands;
    m_executor.SetIsSetting(true);
    m_enemy_executor.SetIsSetting(true);
    TranslateCommands(m_commands);
    TranslateCommands(m_enemy_commands);
    m_executor.SetCommands(m_commands);
    m_enemy_executor.SetCommands(m_enemy_commands);
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
    m_enemy_executor.SetIsSetting(false);
}

void Simulator::SetDirectOrders(const std::vector<Command> &commands,
                                const std::vector<Command> &enemy_commands
#ifdef USE_GRAPHICS
                                ,
                                DebugRenderer *debug_renderer
#endif // USE_GRAPHICS
)
{
    m_commands = commands;
    m_enemy_commands = enemy_commands;
    m_executor.SetIsSetting(true);
    m_enemy_executor.SetIsSetting(true);
    m_executor.SetCommands(m_commands);
    m_enemy_executor.SetCommands(m_enemy_commands);
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
    m_enemy_executor.SetIsSetting(false);
}

void Simulator::SetDirectOrders(const std::vector<std::vector<Command>> &commands
#ifdef USE_GRAPHICS
                                ,
                                DebugRenderer *debug_renderer
#endif // USE_GRAPHICS
)
{
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

void Simulator::SetStartPoint(const std::vector<Command> &commands,
                              const std::vector<Command> &enemy_commands,
                              const ObservationInterface *ob)
{
    CopyAndSetState(ob);
    SetOrders(commands, enemy_commands);
}

const ObservationInterface *Simulator::Observation(int player) const
{
    if (player == 1)
    {
        return m_executor.Observation();
    }
    else if (player = 2 && Coordinator::IsMultiPlayerGame())
    {
        return m_enemy_executor.Observation();
    }
    else
    {
        throw("There is no 2nd players in single player sim or your input is more than 2@Simulator::" + std::string(__FUNCTION__));
    }
}

DebugInterface *Simulator::Debug(int player)
{
    if (player == 1)
    {
        return m_executor.Debug();
    }
    else if (player = 2 && Coordinator::IsMultiPlayerGame())
    {
        return m_enemy_executor.Debug();
    }
    else
    {
        throw("There is no 2nd players in single player sim or your input is more than 2@Simulator::" + std::string(__FUNCTION__));
    }
}

ActionInterface *Simulator::Actions(int player)
{
    if (player == 1)
    {
        return m_executor.Actions();
    }
    else if (player = 2 && Coordinator::IsMultiPlayerGame())
    {
        return m_enemy_executor.Actions();
    }
    else
    {
        throw("There is no 2nd players in single player sim or your input is more than 2@Simulator::" + std::string(__FUNCTION__));
    }
}

ControlInterface *Simulator::Control(int player)
{
    if (player == 1)
    {
        return m_executor.Control();
    }
    else if (player = 2 && Coordinator::IsMultiPlayerGame())
    {
        return m_enemy_executor.Control();
    }
    else
    {
        throw("There is no 2nd players in single player sim or your input is more than 2@Simulator::" + std::string(__FUNCTION__));
    }
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

float Simulator::GetTeamHealthLoss(int player) const
{
    int player_id;
    if (player == 1)
    {
        player_id = m_executor.Observation()->GetPlayerID();
    }
    else if (player == 2)
    {
        player_id = m_enemy_executor.Observation()->GetPlayerID();
    }
    else
    {
        throw("There is no 2nd players in single player sim or your input is more than 2@Simulator::" + std::string(__FUNCTION__));
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

const std::list<Unit> &Simulator::GetTeamDeadUnits(Unit::Alliance alliance, int player) const
{
    if (player == 1)
    {
        return m_executor.GetDeadUnits(alliance);
    }
    else if (player == 2)
    {
        return m_enemy_executor.GetDeadUnits(alliance);
    }
    else
    {
        throw("There is no 2nd players in single player sim or your input is more than 2@Simulator::" + std::string(__FUNCTION__));
    }
}

void Simulator::Load()
{
    if (IsMultiPlayerGame())
    {
        m_relative_units = LoadMultiPlayerGame(m_save, m_executor, *this);
        SetReversedUnitRelation(m_target_to_source_unit_tags, m_relative_units);
        m_executor.Initialize();
        m_enemy_executor.Initialize();
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
    }
    else
    {
        std::cout << "simple load" << std::endl;
        m_executor.Control()->Load();
        // I don't have to rebuild the relationships, the tags will keep
        // unchanged after Save() and Load()
        // I am not sure
    }
}

void Simulator::SetControlledPlayerNum(int controlled_player_num)
{
    if (controlled_player_num == 1)
    {
        SetParticipants(
            {CreateParticipant(Terran, &m_executor)});
    }
    else if (controlled_player_num == 2)
    {
        SetParticipants(
            {CreateParticipant(Terran, &m_executor), CreateParticipant(Terran, &m_enemy_executor)});
    }
    else
    {
        throw("Controlled player number sould be 1 or 2@Simulator::" + std::string(__FUNCTION__));
    }
    m_controlled_player_num = controlled_player_num;
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

void Simulator::TranslateCommands(std::vector<Command> &commands)
{
    for (Command &cmd : commands)
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
}

std::map<Tag, UnitStatisticalData> Simulator::GetUnitsStatistics(int player)
{
    std::map<Tag, UnitStatisticalData> units_statistics;
    const std::map<Tag, UnitStatisticalData> &raw_statistics = (player == 1) ? m_executor.GetUnitsStatistics() : m_enemy_executor.GetUnitsStatistics();
    for (const auto &u : m_save.unit_states) // tag from main process
    {
        units_statistics[u.unit_tag] = raw_statistics.at(m_relative_units[u.unit_tag]->tag);
    }
    return units_statistics;
}

const UnitStatisticalData &Simulator::GetUnitStatistics(Tag tag, int player)
{
    //todo which unit, which player
    if (player == 1)
    {
        return m_executor.GetUnitStatistics(m_relative_units[tag]->tag);
    }
    else if (player == 2)
    {
        return m_enemy_executor.GetUnitStatistics(m_relative_units[tag]->tag);
    }
    else
    {
        throw("There is no 2nd players in single player sim or your input is more than 2@Simulator::" + std::string(__FUNCTION__));
    }
}

GameResult Simulator::CheckGameResult(int player) const
{
    if (player == 1)
    {
        return m_executor.CheckGameResult();
    }
    else if (player == 2)
    {
        return m_enemy_executor.CheckGameResult();
    }
    else
    {
        throw("There is no 2nd players in single player sim or your input is more than 2@Simulator::" + std::string(__FUNCTION__));
    }
}

u_int32_t Simulator::GetEndLoop() const
{
    return m_executor.GetEndLoop();
}

void Simulator::ResetExecutors()
{
    m_executor.Reset();
    m_enemy_executor.Reset();
}