#include "rolling_bot.h"

namespace sc2 {

// void RollingBot::OnGameStart() {
//     // only after game starting I can initialize the ga, or the information will
//     // not be passed to it
//     m_rolling_ga.InitializeObservation(Observation());
// }
// void RollingBot::OnStep() {
//     // after a specific interval, the algorhim should run once
//     if (Observation()->GetGameLoop() % m_interval_size == 0) {
//         // todo first setup the simulator
//         m_rolling_ga.SetSimulatorsStart(Observation());
//         // todo then pass it to algorithm and let algorithm run
//         Solution<Command> sol =
//             m_rolling_ga.Run().front();  // you must control the frames to run
//                                          // in m_sim.Initialize(), not here
//         // todo after running, get the solution to deploy
//         DeploySolution(sol);
//         //? for test
//         std::cout << "deploy!" << std::endl;
//     }
// }
// void RollingBot::DeploySolution(Solution<Command> sol) {
//     for (const Command& c : sol.variable) {
//         // todo before deploying the first command this time, the command queue
//         // should be cleared
//         bool queued_command = true;
//         for (size_t i = 0; i < c.actions.size(); i++) {
//             queued_command = (i == 0) ? false : true;
//             switch (c.actions[i].target_type) {
//                 case ActionRaw::TargetType::TargetNone:
//                     Actions()->UnitCommand(Observation()->GetUnit(c.unit_tag),
//                                            c.actions[i].ability_id,
//                                            queued_command);
//                     break;
//                 case ActionRaw::TargetType::TargetPosition:
//                     Actions()->UnitCommand(Observation()->GetUnit(c.unit_tag),
//                                            c.actions[i].ability_id,
//                                            c.actions[i].target_point,
//                                            queued_command);
//                     break;
//                 case ActionRaw::TargetType::TargetUnitTag:
//                     Actions()->UnitCommand(
//                         Observation()->GetUnit(c.unit_tag),
//                         c.actions[i].ability_id,
//                         Observation()->GetUnit(c.actions[i].TargetUnitTag),
//                         queued_command);
//                     break;
//             }
//         }
//     }
// }
}  // namespace sc2
