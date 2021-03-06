#ifndef ROLLING_BOT_H
#define ROLLING_BOT_H

#include <sc2api/sc2_api.h>
#include <iostream>
#include <string>
#include "simulator.h"
#include "rolling_GA.h"
#include <functional>

namespace sc2 {

    class rolling_bot : public Agent
    {
        using Evaluator = std::function<float(const std::vector<Command>&)>;

    public:
        rolling_bot() = delete;
        //! the only thing this constructor needs to do is to provid all parameters the simulator needs
        //! You need to ensure that all the settings are valid in remote client, especially the map_path (a lot of errors have happend to it)
        rolling_bot(std::string net_address, int port_start, const std::string& process_path,const std::string& map_path):m_rolling_ga(net_address, port_start, process_path, map_path) {};
        //todo I should check if the remote client has been connected
        ~rolling_bot() = default;

        virtual void OnGameStart() override;
        virtual void OnStep() override;

        // Settings
        void SetPopulationSize(int population_size){
            m_rolling_ga.SetPopulationSize(population_size);
        }
        void SetMaxGeneration(int max_generation) {
            m_rolling_ga.SetMaxGeneration(max_generation);
        }
    private:
        //! the function to evaluate the solution
        //std::function<float(const std::vector<Command>&)> evaluator = [&](const std::vector<Command>& sol)->float {
        //    //todo pass the solution to it
        //    m_sim.Load();
        //    //todo let it run for some time
        //    m_sim.Run(m_interval_size / m_sim.GetStepSize());
        //    //todo get the HPs and sum them up as the objective value to return
        //    return m_sim.GetTeamHealthLoss(Unit::Alliance::Enemy) - m_sim.GetTeamHealthLoss(Unit::Alliance::Self);
        //};
        //! the funciton deploy the solution
        void DeploySolution(Solution<Command> sol) {
            for (const Command& c : sol.variable) {
                //todo before deploying the first command this time, the command queue should be cleared
                bool queued_command = true;
                for (size_t i = 0; i < c.actions.size(); i++)
                {
                    queued_command = (i == 0) ? false : true;
                    switch (c.actions[i].target_type)
                    {
                    case ActionRaw::TargetType::TargetNone:
                        Actions()->UnitCommand(Observation()->GetUnit(c.unit_tag), c.actions[i].ability_id, queued_command);
                        break;
                    case ActionRaw::TargetType::TargetPosition:
                        Actions()->UnitCommand(Observation()->GetUnit(c.unit_tag), c.actions[i].ability_id, c.actions[i].target_point, queued_command);
                        break;
                    case ActionRaw::TargetType::TargetUnitTag:
                        Actions()->UnitCommand(Observation()->GetUnit(c.unit_tag), c.actions[i].ability_id, Observation()->GetUnit(c.actions[i].TargetUnitTag), queued_command);
                        break;
                    }
                }
                /*for (const ActionRaw& a : c.actions)
                {
                    switch (a.target_type)
                    {
                    case ActionRaw::TargetType::TargetNone:
                        Actions()->UnitCommand(Observation()->GetUnit(c.unit_tag), a.ability_id, true);
                        break;
                    case ActionRaw::TargetType::TargetPosition:
                        Actions()->UnitCommand(Observation()->GetUnit(c.unit_tag), a.ability_id, a.target_point, true);
                        break;
                    case ActionRaw::TargetType::TargetUnitTag:
                        Actions()->UnitCommand(Observation()->GetUnit(c.unit_tag), a.ability_id, Observation()->GetUnit(a.TargetUnitTag), true);
                        break;
                    }
                }*/
            }
        }

        //! Number of frames for which the algorithm should run once
        int m_interval_size = 80; // about 5 seconds

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

}

#endif // !ROLLING_BOT_H
