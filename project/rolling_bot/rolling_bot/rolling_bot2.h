// #ifndef ROLLING_BOT2_H
// #define ROLLING_BOT2_H

// #include <sc2api/sc2_api.h>
// #include <functional>
// #include <iostream>
// #include <string>
// #include "../simulator/simulator.h"
// #include "../algorithm/rolling_de.h"

// namespace sc2
// {

// class RollingBot2 : public Agent
// {
// public:
//     RollingBot2() = delete;
//     //! the only thing this constructor needs to do is to provid all parameters
//     //! the simulator needs You need to ensure that all the settings are valid
//     //! in remote client, especially the map_path (a lot of errors have happend
//     //! to it)
//     RollingBot2(const std::string &net_address, int port_start,
//                 const std::string &process_path, const std::string &map_path, int max_generation, int population_size = 50)
//         : m_rolling_ea(net_address, port_start, process_path, map_path, max_generation, population_size) {}
//     virtual void OnGameStart() override
//     {
//         m_rolling_ea.Initialize(Observation());
//     }
//     virtual void OnStep() override
//     {
//         //todo after a specific interval, run the algorithm and get the final orders to be given
//         if (Observation()->GetGameLoop() % m_interval_size == 0 && !Observation()->GetUnits(Unit::Alliance::Enemy).empty() && !Observation()->GetUnits(Unit::Alliance::Self).empty())
//         {
//             //  first setup the simulator
//             // m_rolling_ga.SetSimulatorsStart(Observation());
//             //  then pass it to algorithm and let algorithm run
//             std::vector<Command> selected_solution =
//                 m_rolling_ea.Run().front().variable;
//             m_selected_commands = Command::ParseCommands(selected_solution);
//             //? for test
//             std::cout << "deploy!" << std::endl;
//         }
//         // todo if it is not the gameloop to run the algorithm, deploy the command
//         else
//         {
//             Units m_my_team = Observation()->GetUnits(Unit::Alliance::Self);
//             for (const Unit *u : m_my_team)
//             {
//                 bool is_cooling_down = (m_my_team_cooldown_last_frame.find(u->tag) != m_my_team_cooldown_last_frame.end() && std::abs(m_my_team_cooldown_last_frame[u->tag]) < 0.0001f);
//                 if (u->orders.empty() || (is_cooling_down && (m_my_team_cooldown_last_frame[u->tag] < u->weapon_cooldown)) ||
//                     (!is_cooling_down && u->weapon_cooldown > 0.0001f)) // 需要下一个动作
//                 {
//                     if (m_selected_commands.find(u->tag) == m_selected_commands.end())
//                     {
//                         std::cout << "returned solution don't have this unit's commands@" + std::string(__FUNCTION__) << std::endl;
//                     }
//                     std::deque<ActionRaw> &unit_commands = m_selected_commands[u->tag];
//                     if (!m_selected_commands.at(u->tag).empty())
//                     {
//                         const ActionRaw &action = unit_commands.front();
//                         //todo 注入并执行
//                         if (action.ability_id == ABILITY_ID::ATTACK)
//                         {
//                             switch (action.target_type)
//                             {
//                             case ActionRaw::TargetType::TargetNone:
//                             {
//                                 Actions()->UnitCommand(u, action.ability_id);
//                             }
//                             break;
//                             case ActionRaw::TargetType::TargetPosition:
//                             {
//                                 //move threr then attack automatically according to the game AI
//                                 Actions()->UnitCommand(u, ABILITY_ID::MOVE, action.target_point);
//                                 Actions()->UnitCommand(u, action.ability_id, action.target_point, true);
//                             }
//                             break;
//                             case ActionRaw::TargetType::TargetUnitTag:
//                             {
//                                 // directly deploy
//                                 Actions()->UnitCommand(u, action.ability_id, action.target_tag);
//                             }
//                             default:
//                                 break;
//                             }
//                         }
//                         else //! for now, "else" means move action
//                         {
//                             switch (action.target_type)
//                             {
//                             case ActionRaw::TargetType::TargetNone:
//                             {
//                                 Actions()->UnitCommand(u, action.ability_id);
//                             }
//                             break;
//                             case ActionRaw::TargetType::TargetPosition:
//                             {
//                                 Actions()->UnitCommand(u, action.ability_id, action.target_point);
//                             }
//                             break;
//                             case ActionRaw::TargetType::TargetUnitTag:
//                             {
//                                 Actions()->UnitCommand(u, action.ability_id, action.target_tag);
//                             }
//                             default:
//                                 break;
//                             }
//                         }
//                         unit_commands.pop_front();
//                     }
//                 }
//             }
//         }
//     }

//     virtual void OnUnitDestoyed(const Unit *u)
//     {
//         if (Observation()->GetUnits(Unit::Alliance::Self).empty())
//         {
//             std::cout << "No unit to use!" << std::endl;
//             char c;
//             std::cin >> c;
//             //todo I should give control to the main()
//         }
//         else if (Observation()->GetUnits(Unit::Alliance::Enemy).empty())
//         {
//             std::cout << "you win!" << std::endl;
//             char c;
//             std::cin >> c;
//             //todo I should give control to the main()
//         }
//     }

//     // Settings for bot
//     void SetIntervalLength(int frames)
//     {
//         m_interval_size = frames;
//     }

//     RollingEA &Algorithm()
//     {
//         return m_rolling_ea;
//     }

// private:
//     //! the funciton to deploy the solution
//     void DeploySolution(Solution<Command> sol)
//     {
//         for (const Command &c : sol.variable)
//         {
//             // todo before deploying the first command this time, the command
//             // queue should be cleared
//             bool queued_command = true;
//             for (size_t i = 0; i < c.actions.size(); i++)
//             {
//                 queued_command = (i == 0) ? false : true;
//                 switch (c.actions[i].target_type)
//                 {
//                 case ActionRaw::TargetType::TargetNone:
//                     Actions()->UnitCommand(
//                         Observation()->GetUnit(c.unit_tag),
//                         c.actions[i].ability_id, queued_command);
//                     break;
//                 case ActionRaw::TargetType::TargetPosition:
//                     Actions()->UnitCommand(
//                         Observation()->GetUnit(c.unit_tag),
//                         c.actions[i].ability_id, c.actions[i].target_point,
//                         queued_command);
//                     break;
//                 case ActionRaw::TargetType::TargetUnitTag:
//                     Actions()->UnitCommand(
//                         Observation()->GetUnit(c.unit_tag),
//                         c.actions[i].ability_id,
//                         Observation()->GetUnit(c.actions[i].TargetUnitTag),
//                         queued_command);
//                     break;
//                 }
//             }
//         }
//     }

//     //! The algorithm object
//     RollingDE m_rolling_ea;
//      m_selected_commands
//     int m_interval_size = 160;
// };
// } // namespace sc2

// #endif // !ROLLING_BOT2_H