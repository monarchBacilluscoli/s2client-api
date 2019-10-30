#ifndef ROLLING_BOT_H
#define ROLLING_BOT_H

#include <sc2api/sc2_api.h>
#include <functional>
#include <iostream>
#include <string>
#include "../algorithm/rolling_ga.h"
#include "../simulator/simulator.h"
#include "../algorithm/rolling_de.h"

namespace sc2
{

class RollingBot : public Agent
{
public:
    RollingBot() = delete;
    //! the only thing this constructor needs to do is to provid all parameters
    //! the simulator needs You need to ensure that all the settings are valid
    //! in remote client, especially the map_path (a lot of errors have happend
    //! to it)
    RollingBot(const std::string &net_address, int port_start,
               const std::string &process_path, const std::string &map_path, int population_size = 50)
        : m_rolling_ga(net_address, port_start, process_path, map_path, population_size) {}
    virtual void OnGameStart() override
    {
        // only after game starting I can initialize the ga, or the information
        // will not be passed to it
        m_rolling_ga.Initialize(Observation());
        m_my_team = Observation()->GetUnits(Unit::Alliance::Self);
        for (const Unit* u:m_my_team)
        {
            m_my_units_cooldown_last_frame[u->tag] = u->weapon_cooldown;
        }
    }
    virtual void OnStep() override
    {
        //todo after a specific interval, run the algorithm and get the final orders to be given
        if (Observation()->GetGameLoop() % m_interval_size == 0 && !Observation()->GetUnits(Unit::Alliance::Enemy).empty() && !Observation()->GetUnits(Unit::Alliance::Self).empty())
        {
            //  first setup the simulator
            // m_rolling_ga.SetSimulatorsStart(Observation());
            //  then pass it to algorithm and let algorithm run
            m_selected_solution =
                m_rolling_ga.Run()
                    .front().variable; // you must control the frames to run
                              // in m_sim.Initialize(), not here
            //  after running, get the solution to deploy
            DeploySolution(m_selected_solution);
            //? for test
            std::cout << "deploy!" << std::endl;
        }
        // todo if it is not the gameloop to run the algorithm, deploy the command
        else
        {
            Units m_my_team = Observation()->GetUnits(Unit::Alliance::Self);
            for (const Unit *u : m_my_team)
            {
                
            }
            
        }
    }

    virtual void OnUnitDestoyed(const Unit *u)
    {
        if (Observation()->GetUnits().empty())
        {
            std::cout << "No unit to use!" << std::endl;
            char c;
            std::cin >> c;
            //todo I should give control to the main()
        }
    }
    // Settings for bot
    void SetIntervalLength(int frames)
    {
        m_interval_size = frames;
    }

    // // Settings for GA
    void SetPopulationSize(int population_size)
    {
        m_rolling_ga.SetPopulationSize(population_size);
    }
    void SetMaxGeneration(int max_generation)
    {
        m_rolling_ga.SetMaxGeneration(max_generation);
    }
    void SetDebugOn(bool is_debug)
    {
        m_rolling_ga.SetDebugMode(is_debug);
    }

    // Settings for Sims
    void SetSimStepSize(int steps);

private:
    //! the funciton to deploy the solution
    void DeploySolution(Solution<Command> sol)
    {
        for (const Command &c : sol.variable)
        {
            // todo before deploying the first command this time, the command queue should be cleared
            bool queued_command = true;
            for (size_t i = 0; i < c.actions.size(); i++)
            {
                queued_command = (i == 0) ? false : true; // The first command should replace all the commands set to the unit before
                switch (c.actions[i].target_type)
                {
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

    
    int m_interval_size = 160; //! Number of frames for which the algorithm should run once // about 5 seconds
    int m_population_size = 20;
    int m_max_generation = 20;

    //! The algorithm object
    RollingGA m_rolling_ga;

    // data
    Solution<Command> m_selected_solution;
    std::map<Tag, float> m_my_units_cooldown_last_frame;
    Units m_my_team;
};
} // namespace sc2

#endif // !ROLLING_BOT_H