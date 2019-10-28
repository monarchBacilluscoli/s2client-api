#ifndef ROLLING_BOT2_H
#define ROLLING_BOT2_H

#include <sc2api/sc2_api.h>
#include <functional>
#include <iostream>
#include <string>
#include "../simulator/simulator.h"
#include "../algorithm/rolling_de.h"

namespace sc2
{

class RollingBot2 : public Agent
{
public:
    RollingBot2() = delete;
    //! the only thing this constructor needs to do is to provid all parameters
    //! the simulator needs You need to ensure that all the settings are valid
    //! in remote client, especially the map_path (a lot of errors have happend
    //! to it)
    RollingBot2(const std::string &net_address, int port_start,
                const std::string &process_path, const std::string &map_path)
        : m_rolling_ea(net_address, port_start, process_path, map_path, 50, 50) {}
    virtual void OnGameStart() override
    {
        m_rolling_ea.Initialize(Observation());
    }
    virtual void OnStep() override
    {
        // after a specific interval, the algorhim should run once
        if (Observation()->GetGameLoop() % m_interval_size == 0 && !Observation()->GetUnits(Unit::Alliance::Enemy).empty() && !Observation()->GetUnits(Unit::Alliance::Self).empty())
        {
            //  first setup the simulator
            // m_rolling_ea.SetSimulatorsStart(Observation());
            //  then pass it to algorithm and let algorithm run
            Solution<Command> sol =
                m_rolling_ea.Run()
                    .front(); // you must control the frames to run
                              // in m_sim.Initialize(), not here
            //  after running, get the solution to deploy
            DeploySolution(sol);
            //? for test
            std::cout << "deploy!" << std::endl;
        }
    }

    virtual void OnUnitDestoyed(const Unit *u)
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
            std::cout << "you win!" << std::endl;
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

private:
    //! the funciton to deploy the solution
    void DeploySolution(Solution<Command> sol)
    {
        for (const Command &c : sol.variable)
        {
            // todo before deploying the first command this time, the command
            // queue should be cleared
            bool queued_command = true;
            for (size_t i = 0; i < c.actions.size(); i++)
            {
                queued_command = (i == 0) ? false : true;
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

    //! The algorithm object
    RollingDE m_rolling_ea;
    int m_interval_size = 160;
};
} // namespace sc2

#endif // !ROLLING_BOT2_H