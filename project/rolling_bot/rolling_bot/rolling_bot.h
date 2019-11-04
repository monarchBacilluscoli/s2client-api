#ifndef ROLLING_BOT_H
#define ROLLING_BOT_H

#include <queue>
#include <string>
#include <sc2api/sc2_api.h>
#include <functional>
#include <iostream>
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
               const std::string &process_path, const std::string &map_path, int max_generation = 50, int population_size = 50)
        : m_rolling_ga(net_address, port_start, process_path, map_path, max_generation, population_size) {}
    virtual void OnGameStart() override
    {
        // only after game starting I can initialize the ga, or the information
        // will not be passed to it
        m_rolling_ga.Initialize(Observation());
        m_my_team = Observation()->GetUnits(Unit::Alliance::Self);
        for (const Unit *u : m_my_team)
        {
            m_my_team_cooldown_last_frame[u->tag] = u->weapon_cooldown;
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
            std::vector<Command> selected_solution =
                m_rolling_ga.Run().front().variable;
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

    RollingEA& Algorithm(){
        return m_rolling_ga;
    }

    // Settings for GA
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
        m_rolling_ga.SetDebug(is_debug);
    }

    void SetSimLength(int sim_step_length){
        m_rolling_ga.SetSimLength(sim_step_length);
    }

    void SetCommandLength(int command_length){
        m_rolling_ga.SetCommandLength(command_length);
    }

private:
    int m_interval_size = 160; //! Number of frames for which the algorithm should run once // about 5 seconds

    //! The algorithm object
    RollingGA m_rolling_ga;

    // data
    // std::vector<Command> m_selected_solution;
    std::map<Tag, std::deque<ActionRaw>> m_selected_commands;
    std::map<Tag, float> m_my_team_cooldown_last_frame;
    Units m_my_team;
};
} // namespace sc2

#endif // !ROLLING_BOT_H