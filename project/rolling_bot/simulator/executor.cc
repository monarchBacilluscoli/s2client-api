#include <stdexcept>
#include "executor.h"

namespace sc2
{
void Executor::OnStep()
{
    if (m_end_loop == std::numeric_limits<u_int32_t>::max())
    {
        switch (CheckGameResult())
        {
        case GameResult::Win:
        case GameResult::Loss:
        case GameResult::Tie:
            m_end_loop = Observation()->GetGameLoop() - m_start_loop;
            break;
        default:
            break;
        }
    }

    Units mine = Observation()->GetUnits(Unit::Alliance::Self);
    Units allies = Observation()->GetUnits(Unit::Alliance::Ally);
    Units enemies = Observation()->GetUnits(Unit::Alliance::Enemy);
    // on each step, check if each unit has finished its current command, if so, set the next order for it
    if (m_is_setting || m_commands.empty())
    {
        m_last_is_setting = true;
        return;
    }
    if (m_last_is_setting) // but this frame is not
    {
        m_start_loop = Observation()->GetGameLoop();
        m_last_is_setting = false;
    }
    // std::cout << m_commands.size() << "\t" << std::flush;
    for (const Unit *u : mine)
    {
        bool has_cooldown_record = (m_units_states_last_loop.find(u->tag) != m_units_states_last_loop.end());
        // check if the current has been finished
        if (u->orders.empty() ||                                                                                // no order now
            (has_cooldown_record && (m_units_states_last_loop[u->tag].weapon_cooldown < u->weapon_cooldown)) || // this unit has executed a new attack just now
            (!has_cooldown_record && u->weapon_cooldown > 0.f))                                                 // it has shot once but that wasn't recorded
        {
            ++m_units_statistics[u->tag].action_number; // one more action here, then make sure which action happened.
            if (has_cooldown_record && (m_units_states_last_loop[u->tag].weapon_cooldown < u->weapon_cooldown) || (!has_cooldown_record && u->weapon_cooldown > 0.01f))
            {
                ++m_units_statistics[u->tag].attack_number; // know attack number & total number, then I know move number
                m_is_an_attack_to_be_recorded[u->tag] = true;
            }
#ifdef DEBUG
            if (m_commands.find(u->tag) == m_commands.end()) // check if the command for this unit existed
            {
                // std::cout << m_commands.size() << "\t" << std::flush;
                std::cout << "mistake, no commands for this unit@" << __FUNCTION__ << std::endl;
            }
#endif                                          // DEBUG
            if (!m_commands.at(u->tag).empty()) // it there are commands for this unit
            {
                ActionRaw action = m_commands.at(u->tag).front();
                if (action.ability_id == ABILITY_ID::ATTACK_ATTACK || action.ability_id == ABILITY_ID::ATTACK)
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
                else //! for now, "else" means action MOVE
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
        try // Record
        //1. the oldest order is at 0 2. the order keeps there while it is unfinished 3. process is meaningless in normal unit actions
        {
            const Unit &u_last = m_units_states_last_loop[u->tag];                                                                       // if this is the first frame, just add it directly
            if ((u->orders.empty() && !u_last.orders.empty()) || (!u_last.orders.empty() && u->orders.front() != u_last.orders.front())) // if current order is empty but an order has been finished in the last frame, record it or // if current order is not empty but is not the same with the one at last frame
            {
                // if (!u->orders.empty() && !u_last.orders.empty() && u_last.orders.front().ability_id == u->orders.front().ability_id && u_last.orders.front() != u->orders.front())
                // {
                //     std::cout << u->orders.front().ability_id << '\t' << "last: " << u_last.orders.front().target_unit_tag << "\tthis: " << u->orders.front().target_unit_tag << std::endl;
                // }
                if (u_last.orders.front().ability_id == ABILITY_ID::ATTACK && m_is_an_attack_to_be_recorded.find(u->tag) != m_is_an_attack_to_be_recorded.end() && m_is_an_attack_to_be_recorded.at(u->tag) == true) // this attack has been executed successful.
                {
                    m_units_statistics[u->tag].events.actions.emplace_back(Observation()->GetGameLoop() - 1 - m_start_loop, u_last.pos, ABILITY_ID::ATTACK_ATTACK);
                    m_is_an_attack_to_be_recorded.at(u->tag) = false;
                }
                else // not an attack action or attack doesn't take effect (no enemy)
                {
                    m_units_statistics[u->tag].events.actions.emplace_back(Observation()->GetGameLoop() - 1 - m_start_loop, u_last.pos, u_last.orders.front().ability_id);
                }
                // m_units_statistics[u->tag].events.actions.emplace_back(Observation()->GetGameLoop() - 1 - m_start_loop, u_last.pos, u_last.orders.front().ability_id);
            }
            if (u_last.health != u->health)
            {
                m_units_statistics[u->tag].events.health.emplace_back(Observation()->GetGameLoop() - m_start_loop, u->pos, u->health);
            }
            if (u_last.shield > u->shield) // only record the damaged shield value, since it will increase automatically per frame, the store space will be two large //todo make sure the increase rate of shield
            {
                m_units_statistics[u->tag].events.shield.emplace_back(Observation()->GetGameLoop() - m_start_loop, u->pos, u->shield);
            }
        }
        catch (const std::out_of_range &e) // handle the out_of_range exeception of map
        {
            // nothing need to do, after this catch will be a assignment statement
        }
    }
    //todo record statistics of enemies
    for (const Unit *u : Observation()->GetUnits(Unit::Alliance::Enemy))
    {
        if (m_units_states_last_loop.find(u->tag) != m_units_states_last_loop.end() || Observation()->GetGameLoop() % 20 == 0) // 上一帧有这个单位
        {
            if (std::abs(m_units_states_last_loop.at(u->tag).facing - u->facing) > 0.1745)
            {
                m_units_statistics[u->tag].events.actions.emplace_back(Observation()->GetGameLoop() - m_start_loop, u->pos, ABILITY_ID::MOVE);
            }
        }
        else // 上一帧没有这个单位
        {
            m_units_statistics[u->tag].events.actions.emplace_back(Observation()->GetGameLoop() - m_start_loop, u->pos, ABILITY_ID::MOVE);
        }
    }
    RecordLastUnits();
}

void Executor::OnUnitDestroyed(const Unit *unit)
{
    if (!m_is_setting) // only when it isn't in copy and set period the dead units count make sense
    {
        switch (unit->alliance)
        {
        case Unit::Alliance::Ally:
            m_dead_units.ally.push_back(*unit);
            break;
        case Unit::Alliance::Self:
            m_dead_units.self.push_back(*unit);
            break;
        case Unit::Alliance::Enemy:
            m_dead_units.enemy.push_back(*unit);
            break;
        case Unit::Alliance::Neutral:
            m_dead_units.neutral.push_back(*unit);
            break;
        default:
            throw("???@Executor::" + std::string(__FUNCTION__));
            break;
        }
    }
}

void Executor::OnUnitCreated(const Unit *u)
{
    if (!m_is_setting)
    {
        // todo maybe insert new unit states
    }
}

void Executor::SetCommands(const std::vector<Command> &commands)
{
    //todo copy the vector to queue member
    m_commands.clear();
    for (const Command &command : commands)
    {
        std::queue<ActionRaw> &target_command = m_commands[command.unit_tag] = std::queue<ActionRaw>();
        for (const ActionRaw &action : command.actions)
        {
            target_command.push(action);
        }
    }
}

const std::list<Unit> &Executor::GetDeadUnits(Unit::Alliance alliance) const
{
    switch (alliance)
    {
    case Unit::Alliance::Self:
        return m_dead_units.self;
        break;
    case Unit::Alliance::Enemy:
        return m_dead_units.enemy;
        break;
    case Unit::Alliance::Ally:
        return m_dead_units.ally;
        break;
    case Unit::Alliance::Neutral:
        return m_dead_units.neutral;
        break;
    default:
        throw("???@Executor::" + std::string(__FUNCTION__));
        break;
    }
}

const std::map<Tag, UnitStatisticalData> &Executor::GetUnitsStatistics()
{
    // calculate the data first
    for (auto u_st : m_units_statistics)
    {
        m_units_statistics[u_st.first].health_change = CalculateHealthChange(u_st.first);
    }
    return m_units_statistics;
}

const UnitStatisticalData &Executor::GetUnitStatistics(Tag tag)
{
    m_units_statistics[tag].health_change = CalculateHealthChange(tag);
    return m_units_statistics[tag];
}

float Executor::CalculateHealthChange(Tag tag) const
{
    float change;
    if (m_initial_units.at(tag)->is_alive)
    {
        change = m_initial_units.at(tag)->health - m_initial_units_states.at(tag).health;
    }
    else
    {
        change = -m_initial_units_states.at(tag).health;
    }
    return change;
}

GameResult Executor::CheckGameResult() const
{
    if (Observation()->GetUnits(Unit::Alliance::Self).size() + Observation()->GetUnits(Unit::Alliance::Ally).size() == 0)
    {
        if (Observation()->GetUnits(Unit::Alliance::Enemy).size() == 0)
        {
            return GameResult::Tie;
        }
        return GameResult::Loss;
    }
    else if (Observation()->GetUnits(Unit::Alliance::Enemy).size() == 0)
    {
        return GameResult::Win;
    }
    else
    {
        return GameResult::Undecided;
    }
}

u_int32_t Executor::GetEndLoop() const
{
    return m_end_loop;
}

void Executor::SetIsSetting(bool is_setting)
{
    m_is_setting = is_setting;
}

void Executor::Clear()
{
    ClearCommands();
    ClearUnitsData();
    m_end_loop = std::numeric_limits<u_int32_t>::max();
}

void Executor::RecordLastUnits()
{
    for (const Unit *u : Observation()->GetUnits())
    {
        m_units_states_last_loop[u->tag] = *u;
    }
}

void Executor::ClearCommands()
{
    m_commands.clear();
}

void Executor::ClearUnitsData()
{
    ClearDeadUnits();
    m_initial_units.clear();
    m_units_statistics.clear();
    m_initial_units_states.clear();
    m_units_states_last_loop.clear();
    m_is_an_attack_to_be_recorded.clear();
}

void Executor::ClearDeadUnits()
{
    m_dead_units.self.clear();
    m_dead_units.enemy.clear();
    m_dead_units.ally.clear();
    m_dead_units.neutral.clear();
}

void Executor::Initialize()
{
    Clear();
    InitUnitStatistics(Observation()->GetUnits());
    RecordInitialUnitsStates(Observation()->GetUnits());
}

void Executor::InitUnitStatistics(const Units &all_units)
{
    for (const auto &u : all_units)
    {
        m_initial_units[u->tag] = u;
        m_units_statistics[u->tag] = UnitStatisticalData();
        m_initial_units_states[u->tag] = *u;
    }
}

void Executor::RecordInitialUnitsStates(const Units &all_units)
{
    for (const auto &u : all_units)
    {
        m_initial_units_states[u->tag] = *u;
    }
}
} // namespace sc2