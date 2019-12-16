#include "rolling_bot.h"
#include "sc2api/sc2_score.h"
#include <sstream>

namespace sc2
{
void RollingBot::OnGameStart()
{
    // only after game starting I can initialize the ga, or the information
    // will not be passed to it
    m_rolling_ea.Initialize(Observation());
    m_my_team = Observation()->GetUnits(Unit::Alliance::Self);
    for (const Unit *u : m_my_team)
    {
        m_my_team_cooldown_last_frame[u->tag] = u->weapon_cooldown;
    }
}

void RollingBot::OnStep()
{
    //todo after a specific interval, run the algorithm and get the final orders to be given
    if (Observation()->GetGameLoop() % m_interval_size == 0 && !Observation()->GetUnits(Unit::Alliance::Enemy).empty() && !Observation()->GetUnits(Unit::Alliance::Self).empty())
    {
        //  first setup the simulator
        // m_rolling_ea.SetSimulatorsStart(Observation());
        //  then pass it to algorithm and let algorithm run
        //todo select the solution whose difference between the two objs is the biggest
        std::vector<Command> selected_solution;
        RollingEA::Population pop = m_rolling_ea.Run();
        selected_solution = m_solution_selector(pop).variable;
        m_selected_commands = Command::ParseCommands(selected_solution);
        //? for test
        std::cout << "deploy!" << std::endl;
    }
    // todo if it is not the gameloop to run the algorithm, deploy the command
    else
    {
        Units m_my_team = Observation()->GetUnits(Unit::Alliance::Self);
        for (const Unit *u : m_my_team)
        {
            bool is_cooling_down = (m_my_team_cooldown_last_frame.find(u->tag) != m_my_team_cooldown_last_frame.end() && std::abs(m_my_team_cooldown_last_frame[u->tag]) < 0.0001f);
            if (u->orders.empty() || (is_cooling_down && (m_my_team_cooldown_last_frame[u->tag] < u->weapon_cooldown)) ||
                (!is_cooling_down && u->weapon_cooldown > 0.0001f)) // 需要下一个动作
            {
                if (m_selected_commands.find(u->tag) == m_selected_commands.end())
                {
                    std::cout << "returned solution don't have this unit's commands@" + std::string(__FUNCTION__) << std::endl;
                }
                std::deque<ActionRaw> &unit_commands = m_selected_commands[u->tag];
                if (!m_selected_commands.at(u->tag).empty())
                {
                    const ActionRaw &action = unit_commands.front();
                    //todo 注入并执行
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
                    unit_commands.pop_front();
                }
            }
        }
    }
}

void RollingBot::OnUnitDestroyed(const Unit *u)
{
    if (Observation()->GetUnits(Unit::Alliance::Self).empty())
    {
        std::cout << "No unit to use!" << std::endl;
        char c;
        std::cin >> c;
        //todo I should give control to the main()
    }
    else if (Observation()->GetUnits(Unit::Alliance::Enemy).empty())
    {
        std::cout << "Looks that you have won? " << std::endl;
        char c;
        std::cin >> c;
    }
}

void RollingBot::OnGameEnd()
{
    Score score = Observation()->GetScore();
    time_t recording_time = OutputGameScore(score, "scores/test_scores.txt", m_remark);
    std::stringstream ss;
    ss << std::put_time(gmtime(&recording_time), "%Y_%m_%d_%H_%M_%S");
    std::string path = std::string("replays/") + ss.str() + ".SC2Replay";
    Control()->SaveReplay(path);
    std::cout << "Replay saved!" << std::endl;
    if (m_current_round < m_game_round)
    {
        std::cout << __FUNCTION__ << std::endl;
        ++m_current_round;
        AgentControl()->Restart(); // It doesn't work in multi-player game
    }
}

void RollingBot::SetIntervalLength(int frames)
{
    m_interval_size = frames;
}

void RollingBot::SetStyle(PLAY_STYLE style)
{
    switch (style)
    {
    case PLAY_STYLE::IRONHEAD:
    case PLAY_STYLE::AGGRESSIVE:
        m_solution_selector = &RollingBot::SelectMostIronHeadSolution;
        break;
    case PLAY_STYLE::RUNAWAY:
    case PLAY_STYLE::DEFENSIVE:
        m_solution_selector = &RollingBot::SelectMostRunAwaySolution;
        break;
    case PLAY_STYLE::NORMAL:
    default:
        m_solution_selector = &RollingBot::SelectMostOKSolution;
        break;
    }
}

void RollingBot::SetGameRound(int round)
{
    m_game_round = round;
}

void RollingBot::SetRemark(const std::string &remark)
{
    m_remark = remark;
}

RollingEA &RollingBot::Algorithm()
{
    return m_rolling_ea;
}

const Solution<Command> &RollingBot::SelectMostIronHeadSolution(const Population &pop)
{
    if (pop.empty())
    {
        throw("The pop passed here is an empty pop.@RollingBot::" + std::string(__FUNCTION__));
    }
    Population::const_iterator it = std::max_element(pop.begin(), pop.end(), [](const Solution<Command> &largetest, const Solution<Command> &first) {
        return largetest.objectives[0] < first.objectives[0];
    });
    return *it;
}

const Solution<Command> &RollingBot::SelectMostRunAwaySolution(const Population &pop)
{
    if (pop.empty())
    {
        throw("The pop passed here is an empty pop.@RollingBot::" + std::string(__FUNCTION__));
    }
    Population::const_iterator it = std::max_element(pop.begin(), pop.end(), [](const Solution<Command> &largetest, const Solution<Command> &first) {
        return largetest.objectives[1] < first.objectives[1];
    });
    return *it;
}

const Solution<Command> &RollingBot::SelectMostOKSolution(const Population &pop)
{
    if (pop.empty())
    {
        throw("The pop passed here is an empty pop.@RollingBot::" + std::string(__FUNCTION__));
    }
    Population::const_iterator it = std::max_element(pop.begin(), pop.end(), [](const Solution<Command> &largetest, const Solution<Command> &first) {
        return (std::abs(largetest.objectives[0]) - std::abs(largetest.objectives[1])) < (std::abs(first.objectives[0]) - std::abs(first.objectives[1]));
    });
    return *it;
}

} // namespace sc2
