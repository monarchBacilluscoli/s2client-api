#include <stdexcept>
#include "executor.h"

namespace sc2
{
void Executor::OnStep()
{
    // on each step, check if each unit has finished its current command, if so, set the next order for it
    if (m_is_setting || m_commands.empty())
    {
        return;
    }
    // std::cout << m_commands.size() << "\t" << std::flush;
    Units my_team = Observation()->GetUnits(Unit::Alliance::Self);
    for (const Unit *u : my_team)
    {
        bool has_cooldown_record = (m_cooldown_last_frame.find(u->tag) != m_cooldown_last_frame.end());
        // check if the current has been finished
        if (u->orders.empty() ||                                                             // no order now
            (has_cooldown_record && (m_cooldown_last_frame[u->tag] < u->weapon_cooldown)) || // this unit has executed a new attack just now
            (!has_cooldown_record && u->weapon_cooldown > 0.f))                              // it has shot once but that wasn't recorded
        {
            ++m_units_statistics[u->tag].action_number;
            if (has_cooldown_record && (m_cooldown_last_frame[u->tag] < u->weapon_cooldown) || (!has_cooldown_record && u->weapon_cooldown > 0.01f))
            {
                ++m_units_statistics[u->tag].attack_number;
            }
#ifdef DEBUG
            // check if the command for this unit existed
            if (m_commands.find(u->tag) == m_commands.end())
            {
                // std::cout << m_commands.size() << "\t" << std::flush;
                std::cout << "mistake, no commands for this unit@" << __FUNCTION__ << std::endl;
            }
#endif // DEBUG
            if (!m_commands.at(u->tag).empty())
            {
                ActionRaw action = m_commands.at(u->tag).front();
                //todo distinguish the attack action and move action
                if (action.ability_id == ABILITY_ID::ATTACK_ATTACK)
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
        try
        {
            const Unit &u_last = m_units_states_last_loop[u->tag];
            if (u->orders.empty()) // 1. the oldest order is at 0 2. the order keeps there while it is unfinished 3. process is meaningless in normal unit actions
            {
                if (!u_last.orders.empty())
                {
                    m_units_statistics[u->tag].events.actions.emplace_back(Observation()->GetGameLoop(), u_last.orders.front().ability_id);
                }
            }
            else if (!u_last.orders.empty() && u->orders.front() != u_last.orders.front())
            {
                m_units_statistics[u->tag].events.actions.emplace_back(Observation()->GetGameLoop(), u_last.orders.front().ability_id);
            }
            else if (u_last.weapon_cooldown < u->weapon_cooldown) // attack need to be recorded specially since only if the target has been dead, or the attack order will not be ended
            {
                m_units_statistics[u->tag].events.actions.emplace_back(Observation()->GetGameLoop(), ABILITY_ID::ATTACK_ATTACK);
                // std::cout << "attack at loop " << Observation()->GetGameLoop();
                Actions()->SendChat(std::string("attack at loop ") + std::to_string(Observation()->GetGameLoop()));
            }
            // record states
            if (u_last.health != u->health)
            {
                m_units_statistics[u->tag].events.health.emplace_back(Observation()->GetGameLoop(), u->health);
            }
            if (u_last.shield > u->shield) // only record the damaged shield value, since it will increase automatically per frame, the store space will be two large //todo make sure the increase rate of shield
            {
                m_units_statistics[u->tag].events.shield.emplace_back(Observation()->GetGameLoop(), u->shield);
            }
        }
        catch (const std::out_of_range &e) // handle the out_of_range exeception of map
        {
            // nothing need to do, after this catch will be a assignment statement
        }
        m_units_states_last_loop[u->tag] = *u;
        m_cooldown_last_frame[u->tag] = u->weapon_cooldown;
    }
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

void Executor::SetIsSetting(bool is_setting)
{
    m_is_setting = is_setting;
}

void Executor::Clear()
{
    ClearCommands();
    ClearCooldownData(); //todo can be deleted, since it is included in units_states_last_loop
    ClearUnitsData();
}

void Executor::ClearCommands()
{
    m_commands.clear();
}

void Executor::ClearCooldownData()
{
    m_cooldown_last_frame.clear();
}

void Executor::ClearUnitsData()
{
    ClearDeadUnits();
    m_initial_units.clear();
    m_units_statistics.clear();
    m_initial_units_states.clear();
    m_units_states_last_loop.clear();
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