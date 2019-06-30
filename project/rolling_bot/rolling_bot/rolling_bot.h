#ifndef ROLLING_BOT_H
#define ROLLING_BOT_H

#include <sc2api/sc2_api.h>
#include <functional>
#include <iostream>
#include <string>
#include "../algorithm/rolling_ga.h"
#include "../simulator/simulator.h"

namespace sc2 {

class RollingBot : public Agent {
    using Evaluator = std::function<float(const std::vector<Command>&)>;

   public:
    RollingBot() = delete;
    //! the only thing this constructor needs to do is to provid all parameters
    //! the simulator needs You need to ensure that all the settings are valid
    //! in remote client, especially the map_path (a lot of errors have happend
    //! to it)
    RollingBot(const std::string& net_address, int port_start,
               const std::string& process_path, const std::string& map_path)
        : m_rolling_ga(net_address, port_start, process_path, map_path) {}
    virtual void OnGameStart() override {
        // only after game starting I can initialize the ga, or the information
        // will not be passed to it
        m_rolling_ga.InitializeObservation(Observation());
    }
    virtual void OnStep() override {
        // after a specific interval, the algorhim should run once
        if (Observation()->GetGameLoop() % m_interval_size == 0) {
            // todo first setup the simulator
            m_rolling_ga.SetSimulatorsStart(Observation());
            // todo then pass it to algorithm and let algorithm run
            Solution<Command> sol =
                m_rolling_ga.Run()
                    .front();  // you must control the frames to run
                               // in m_sim.Initialize(), not here
            // todo after running, get the solution to deploy
            DeploySolution(sol);
            //? for test
            std::cout << "deploy!" << std::endl;
        }
    }

    // Settings
    void SetPopulationSize(int population_size) {
        m_rolling_ga.SetPopulationSize(population_size);
    }
    void SetMaxGeneration(int max_generation) {
        m_rolling_ga.SetMaxGeneration(max_generation);
    }

   private:
    //! the funciton to deploy the solution
    void DeploySolution(Solution<Command> sol) {
        for (const Command& c : sol.variable) {
            // todo before deploying the first command this time, the command
            // queue should be cleared
            bool queued_command = true;
            for (size_t i = 0; i < c.actions.size(); i++) {
                queued_command = (i == 0) ? false : true;
                switch (c.actions[i].target_type) {
                    case ActionRaw::TargetType::TargetNone:
                        Actions()->UnitCommand(
                            Observation()->GetUnit(c.unit_tag),
                            c.actions[i].ability_id, queued_command);
                        break;
                    case ActionRaw::TargetType::TargetPosition:
                        Actions()->UnitCommand(
                            Observation()->GetUnit(c.unit_tag),
                            c.actions[i].ability_id, c.actions[i].target_point,
                            queued_command);
                        break;
                    case ActionRaw::TargetType::TargetUnitTag:
                        Actions()->UnitCommand(
                            Observation()->GetUnit(c.unit_tag),
                            c.actions[i].ability_id,
                            Observation()->GetUnit(c.actions[i].TargetUnitTag),
                            queued_command);
                        break;
                }
            }
        }
    }

    //! Number of frames for which the algorithm should run once
    int m_interval_size = 80;  // about 5 seconds
    //
    int m_population_size = 20;
    //
    int m_max_generation = 20;

    //! the simulator which is used as a forward model...
    Simulator m_sim;
    std::vector<Simulator> m_simulators;
    //! The algorithm object
    RollingGA m_rolling_ga;
};
}  // namespace sc2

#endif  // !ROLLING_BOT_H