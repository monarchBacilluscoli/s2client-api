#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "../../global_defines.h"

#include <sc2api/sc2_api.h>
#include <string>
#include <thread>
#include "debug_renderer/debug_renderer.h"
#include "../algorithm/rolling_solution.h"
#include "state.h"
#include "executor.h"

namespace sc2
{
        class Simulator : public Coordinator
        {

        public:
                Simulator() = default;
                ~Simulator() = default;

                // if you want the both sides' players are under your control, input 2
                void SetControlledPlayerNum(int controlled_player_num = 1);

                // set your opponent as a user-defined bot
                void SetOpponent(Agent *agent);
                // set your opponent as a built-in bot - a computer
                void SetOpponent(Difficulty difficulty);

                // copys state and sets orders for preparation to run
                void SetStartPoint(const std::vector<Command> &commands, const ObservationInterface *ob);                                             // for single-player compatibility
                void SetStartPoint(const std::vector<Command> &commands, const std::vector<Command> &enemy_commands, const ObservationInterface *ob); // for multi-player
                // direct send the orders to team 1 (self team) units
                void SetOrders(const std::vector<Command> &commands, const std::vector<Command> &enemy_commands = std::vector<Command>()
#ifdef USE_GRAPHICS // this graohics may be used for debug
                                                                         ,
                               DebugRenderer *debug_renderer = nullptr
#endif // USE_GRAPHICS
                );
                //todo set both sides' orders
                void SetOrders(const std::vector<std::vector<Command>> &commands
#ifdef USE_GRAPHICS // this graohics may be used for debug
                               ,
                               DebugRenderer *debug_renderer = nullptr
#endif // USE_GRAPHICS
                );
                void SetDirectOrders(const std::vector<Command> &commands, const std::vector<Command> &enemy_commands = std::vector<Command>()
#ifdef USE_GRAPHICS
                                                                               ,
                                     DebugRenderer *debug_renderer = nullptr
#endif // USE_GRAPHICS
                );
                //todo set both sides' direct orders
                void SetDirectOrders(const std::vector<std::vector<Command>> &commands
#ifdef USE_GRAPHICS
                                     ,
                                     DebugRenderer *debug_renderer = nullptr
#endif // USE_GRAPHICS
                );

                // copy the game state from a specific game observation.
                void CopyAndSetState(const ObservationInterface *ob
#ifdef USE_GRAPHICS
                                     ,
                                     DebugRenderer *debug_renderer = nullptr
#endif             // USE_GRAPHICS
                ); // the debug_renderer was only used to debug this function when I worte it.
                // runs for specific number of steps which can be set by user
                std::thread::id Run(int steps
#ifdef USE_GRAPHICS
                                    ,
                                    DebugRenderer *debug_renderer = nullptr
#endif // USE_GRAPHICS
                );

                // load the copied state
                void Load();
                // exposes ObservationInterface of the executors, 1 means the self player, 2 means the enemy player
                const ObservationInterface *Observation(int player = 1) const;
                // exposes DebugInterface of the executors, 1 means the self player, 2 means the enemy player
                DebugInterface *Debug(int player = 1);
                // exposes ActionInterface of the executors, 1 means the self player, 2 means the enemy player
                ActionInterface *Actions(int player = 1);
                // exposes ControlInterface of the executors, 1 means the self player, 2 means the enemy player
                ControlInterface *Control(int player = 1);

                std::vector<Command> GetOrders(int player = 1) { return player == 1 ? m_commands : m_enemy_commands; }
                std::vector<Command> GetOriginalOrders(int player = 1) { return player == 1 ? m_original_commands : m_original_enemy_commands; } // get the original orders (sent and stored, raw orders)
                const std::map<Tag, const Unit *> &GetRelativeUnits(int player = 1) const { return player == 1 ? m_relative_units : m_relative_units_enemy; }
                const State &GetSave() const { return m_save; }

                float GetTeamHealthLoss(Unit::Alliance alliance) const;                                 // get health loss result
                float GetTeamHealthLoss(int player = 1) const;                                          // get health loss result
                const std::list<Unit> &GetTeamDeadUnits(Unit::Alliance alliance, int player = 1) const; // get dead units result
                std::map<Tag, UnitStatisticalData> GetUnitsStatistics(int player = 1);
                const UnitStatisticalData &GetUnitStatistics(Tag tag, int player = 1);

                // check if the game is end by either side winning
                GameResult CheckGameResult(int player = 1) const;
                u_int32_t GetEndLoop() const;

                void ResetExecutors(); // used for crash handle of simulator

        private:
                int m_controlled_player_num = 1;
                // the bot to be called outside to send orders or get observations
                Executor m_executor;
                Executor m_enemy_executor;
                // save of state
                State m_save;
                // orders sent in
                std::vector<Command> m_original_commands;
                std::vector<Command> m_original_enemy_commands;
                // translated orders by local unit tags rather than original tags
                std::vector<Command> m_commands;
                std::vector<Command> m_enemy_commands;

                std::map<Tag, const Unit *> m_relative_units; // form source tag to target unit //todo check if opponent's is included
                std::map<Tag, const Unit *> m_relative_units_enemy;
                std::map<Tag, Tag> m_target_to_source_unit_tags;       // from target to source unit tags
                std::map<Tag, Tag> m_target_to_source_unit_tags_enemy; // from target to source unit tags

                // set units relations#
                // so the caller of this simultor doesn't have to know the tags of units
                // in simulator here
                void SetUnitsRelations(State state, Units us_copied);
                // set reversed unit tags for (?)
                void SetReversedUnitRelation(std::map<Tag, Tag> &target_to_source_units, const std::map<Tag, const Unit *> &relative_units);
                // translate the tags in those commands into local tags
                void TranslateCommands(std::vector<Command> &commands); // safe in 2 players sim, since unit tags are same on both sides

        public:
                // add "Sim" after the original map name
                static std::string GenerateSimMapPath(const std::string &map_path);
        };

} // namespace sc2

#endif // SIMULATOR_H