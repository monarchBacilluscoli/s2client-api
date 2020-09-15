#include <sstream>
#include <functional>
#include <iostream>

#include "sc2api/sc2_score.h"
#include "rolling_bot.h"
#include "debug_use.h"

namespace sc2
{

    const std::vector<std::string> g_play_style_names = {
        "NORMAL",
        "IRONHEAD",
        "AGGRESSIVE",
        "DEFENSIVE",
        "RUNAWAY"};

    void RollingBot::OnGameStart()
    {
        if (m_is_output_conver)
        {
            std::string conver_path = CurrentFolder() + "/conver_data.txt";
            std::fstream conver_file(conver_path, std::ios::out | std::ios::app);
            conver_file << "// " << m_rolling_ea.GetObjectiveSize() << m_rolling_ea.IsUsePriori() << m_rolling_ea.IsUseFixByData() << m_rolling_ea.IsUseAssemble() << '\t' << m_rolling_ea.GetSimLength() << '\t' << m_rolling_ea.GetSimLength() << '\t' << m_interval_size << std::endl;
            conver_file.close();
        }

        // only after game starting I can initialize the ga, or the information
        // will not be passed to it
        m_rolling_ea.Initialize(Observation());
        m_my_initial_team = Observation()->GetUnits(Unit::Alliance::Self);
        for (const Unit *u : m_my_initial_team)
        {
            m_my_team_cooldown_last_frame[u->tag] = u->weapon_cooldown;
        }
        {
            //Run algorithm once
            SetCommandFromAlgorithm(); // The first run of algorithm on game start
        }

        if (m_is_output_sim_record)
        {
            // output the final pop
            std::string output_file_path = CurrentFolder() + "/sim_record.txt";
            std::fstream sim_fs = std::fstream(output_file_path, std::ios::app | std::ios::out);
            sim_fs << "//" << m_rolling_ea.GetObjectiveSize() << m_rolling_ea.IsUsePriori() << m_rolling_ea.IsUseFixByData() << m_rolling_ea.IsUseAssemble() << '\t' << m_rolling_ea.GetSimLength() << std::endl;
            m_rolling_ea.OutputPopulationSimResult(sim_fs);
            sim_fs.close();
        }

        if (m_is_only_start)
        {
            AgentControl()->Restart(); // for test
        }
        return;
    }

    void RollingBot::OnStep()
    {
        if (Observation()->GetGameLoop() == 0) //! gameloop 0 should be skipped since 1. OnGameStart() has done the thing 2. OnStep is called twice on loop 0
        {
            return;
        }
        // after a specific interval, run the algorithm and get the final orders to be given
        if (Observation()->GetGameLoop() % m_interval_size == 0 && !Observation()->GetUnits(Unit::Alliance::Enemy).empty() && !Observation()->GetUnits(Unit::Alliance::Self).empty())
        {
            Control()->SaveReplay(CurrentFolder() + "/fix_test.SC2Replay");

            { // stop all the units first, or it may cause some problem since the simulation starts from the condition where all units are still
                Units my_team = Observation()->GetUnits(Unit::Alliance::Self);
                Actions()->UnitCommand(my_team, ABILITY_ID::STOP_STOP);
            }
            SetCommandFromAlgorithm();
        }
        // if it is not the gameloop to run the algorithm, deploy the command
        else
        {
            Units my_team = Observation()->GetUnits(Unit::Alliance::Self);
            for (const Unit *u : my_team)
            {
                bool has_cooldown_record = (m_my_team_cooldown_last_frame.find(u->tag) != m_my_team_cooldown_last_frame.end() /*&& std::abs(m_my_team_cooldown_last_frame[u->tag]) > 1.f*/);
                if (u->orders.empty() ||
                    (has_cooldown_record && (m_my_team_cooldown_last_frame[u->tag] - u->weapon_cooldown) < -1e-7) ||
                    (!has_cooldown_record && u->weapon_cooldown > 0.f)) // 需要下一个动作
                {
                    if (m_selected_commands.find(u->tag) == m_selected_commands.end()) // 当前地图游戏中的单位在命令序列中的对应命令如果不存在
                    {
                        std::cout << "returned solution don't have this unit's commands@" + std::string(__FUNCTION__) << std::endl;
                    }
#ifdef DEBUG
                    Actions()->SendChat("one action!");
#endif // DEBUG
                    std::deque<ActionRaw> &unit_commands = m_selected_commands[u->tag];
                    if (!unit_commands.empty()) // 如果当前单位的动作队列不为空
                    {
                        const ActionRaw &action = unit_commands.front();
                        //todo 注入并执行
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
                m_my_team_cooldown_last_frame[u->tag] = u->weapon_cooldown;
            }
        }
    }

    void RollingBot::OnUnitDestroyed(const Unit *u)
    {
        if (Observation()->GetUnits(Unit::Alliance::Self).empty())
        {
            std::cout << "No unit to use!" << std::endl;
        }
        else if (Observation()->GetUnits(Unit::Alliance::Enemy).empty())
        {
            std::cout << "Looks that you have won? " << std::endl;
        }
    }

    void RollingBot::OnGameEnd()
    {
        time_t recording_time = OutputGameResult(Observation(), "scores/test_scores.txt", m_remark);
        std::stringstream ss;
        ss << std::put_time(gmtime(&recording_time), "%Y_%m_%d_%H_%M_%S");
        std::string path = std::string("sc2replays/") + ss.str() + ".SC2Replay";
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

    void RollingBot::SetCommandFromAlgorithm()
    {
        RollingSolution<Command> selected_solution;
        Actions()->SendChat(std::string("Run algorithm in game loop ") + std::to_string(Observation()->GetGameLoop()));
        std::cout << std::string("Run algorithm in game loop ") + std::to_string(Observation()->GetGameLoop()) << std::endl;
        m_rolling_ea.Initialize(Observation());
        RollingEA::Population pop = m_rolling_ea.Run(); // Run the algorithm
        // output the final pop
        // output the longest actions
        // std::fstream stat_record("/home/liuyongfeng/s2client-api-liu/s2client-api/project/rolling_bot/rolling_bot/stat.txt", std::ios::out | std::ios::app);
        // OutputAllStatistics(pop, stat_record);

        selected_solution = m_solution_selector(pop);

        // std::fstream solution_record(CurrentFolder() + "/solution_record.txt", std::ios::out | std::ios::app);
        // OutputSolution(raw_selected_solution, solution_record);
        // solution_record << std::endl
        //                 << std::endl;
        //! check all the events
        // std::fstream stat_record(CurrentFolder() + "/stat_record.txt", std::ios::out | std::ios::app);
        // std::fstream solution_record(CurrentFolder() + "/solution_record.txt", std::ios::out | std::ios::app);
        // OutputEvents(selected_solution, stat_record);
        // OutputSolution(selected_solution, solution_record);
        // selected_solution.GetUnitPossiablePosition(Observation()->GetUnits(Unit::Alliance::Enemy).front()->tag, 97);

        std::fstream stat_record(CurrentFolder() + "/stat_record.txt", std::ios::out | std::ios::app);
        // OutputEvents(selected_solution, stat_record);
        // OutputSolution(selected_solution, solution_record);

        m_selected_commands = Command::ConmmandsVecToDeque(selected_solution.variable); // transfor command vector to deque for easy to use

        Actions()->SendChat("Number of enemies: " + std::to_string(Observation()->GetUnits(Unit::Alliance::Enemy).size()));
        Actions()->SendChat("Algorithm finished run after " + std::to_string(m_rolling_ea.GetCurrentGeneration()) + "generations");
        std::string objs_str = std::string("deploy, objs: ") + std::move(ContainerToStringWithSeparator(selected_solution.objectives));
        Actions()->SendChat(objs_str);
        std::cout << objs_str << std::endl;
    }

    const RollingSolution<Command> &RollingBot::SelectMostIronHeadSolution(const Population &pop)
    {
        if (pop.empty())
        {
            throw("The pop passed here is an empty pop.@RollingBot::" + std::string(__FUNCTION__));
        }
        Population::const_iterator it = std::max_element(pop.begin(), pop.end(), [](const RollingSolution<Command> &largetest, const RollingSolution<Command> &first) {
            return largetest.objectives[0] < first.objectives[0];
        });
        return *it;
    }

    const RollingSolution<Command> &RollingBot::SelectMostRunAwaySolution(const Population &pop)
    {
        if (pop.empty())
        {
            throw("The pop passed here is an empty pop.@RollingBot::" + std::string(__FUNCTION__));
        }
        Population::const_iterator it = std::max_element(pop.begin(), pop.end(), [](const RollingSolution<Command> &largetest, const RollingSolution<Command> &first) {
            return largetest.objectives[1] < first.objectives[1]; // the maximum of loss (negetive)
        });
        return *it;
    }

    const RollingSolution<Command> &RollingBot::SelectMostOKSolution(const Population &pop)
    {
        // float total_enemy_health = GetTotalHealth(Observation()->GetUnits(Unit::Alliance::Enemy));
        // float total_my_health = GetTotalHealth(Observation()->GetUnits(Unit::Alliance::Self));
        // if (pop.empty())
        // {
        //     throw("The pop passed here is an empty pop.@RollingBot::" + std::string(__FUNCTION__));
        // }
        // //todo Here I need another version for 2
        // Population::const_iterator it = std::max_element(pop.begin(), pop.end(), [total_enemy_health, total_my_health](const RollingSolution<Command> &current_largetest, const RollingSolution<Command> &first) {
        //     if ((std::abs(current_largetest.objectives[0]) - std::abs(current_largetest.objectives[1])) - (std::abs(first.objectives[0]) - std::abs(first.objectives[1])) > 0.0001f)
        //     {
        //         return (std::abs(current_largetest.objectives[0]) / total_enemy_health - std::abs(current_largetest.objectives[1]) / total_my_health) < (std::abs(first.objectives[0]) / total_enemy_health - std::abs(first.objectives[1]) / total_my_health); // the first obj is gain, the second obj is loss
        //     }
        //     else
        //     {
        //         return current_largetest.results.front().game.end_loop > first.results.front().game.end_loop;
        //     }
        // });
        // return *it;
        if (pop.empty())
        {
            throw("The pop passed here is an empty pop.@RollingBot::" + std::string(__FUNCTION__));
        }
        Population::const_iterator it = std::max_element(pop.begin(), pop.end(), [](const RollingSolution<Command> &current_largetest, const RollingSolution<Command> &first) {
            if ((std::abs(current_largetest.objectives[0]) - std::abs(current_largetest.objectives[1])) - (std::abs(first.objectives[0]) - std::abs(first.objectives[1])) > 0.0001f)
            {
                return (std::abs(current_largetest.objectives[0]) - std::abs(current_largetest.objectives[1])) < (std::abs(first.objectives[0]) - std::abs(first.objectives[1])); // the first obj is gain, the second obj is loss
            }
            else
            {
                return current_largetest.results.front().game.end_loop > first.results.front().game.end_loop;
            }
        });
        return *it;
    }

} // namespace sc2
