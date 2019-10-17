#ifndef SIMULATOR_POOL_H
#define SIMULATOR_POOL_H

#include <future>
#include <chrono>
#include <string>
#include <vector>
#include <thread>
#include "simulator.h"

namespace sc2
{

// I must lay aside all things about a thread, that is the thread result holder and simulator obj, so I'd better putting them into one class
template <typename T>
struct Simulation
{
    Simulator sim;
    // T is used to hold result type, but for now, void is enough
    std::future<T> result_holder;
};

class SimulatorPool
{
public:
    SimulatorPool() = default;
    SimulatorPool(int size, const std::string &net_address, int port_start, const std::string &process_path, const std::string &map_path) : m_simulations(size),
                                                                                                                                            m_sol_sim_map(size),
                                                                                                                                            m_net_address(net_address),
                                                                                                                                            m_port_start(port_start),
                                                                                                                                            m_process_path(process_path),
                                                                                                                                            m_map_path(map_path)
    {
        int i=0;
        for (Simulation<void> &simulation : m_simulations)
        {
            Simulator &sim = simulation.sim;
            //sim.SetNetAddress(m_net_address);
            sim.SetPortStart(port_start);
            sim.SetProcessPath(process_path);
            sim.SetMapPath(map_path);
            sim.SetStepSize(1);
            m_sol_sim_map[i] = &simulation;
            port_start += 2;
        }
        m_port_end = port_start;
    }
    // if you use default constructor you must call this function to set the sims
    void SetSimsAsyc(int size, const std::string &net_address, int port_start, const std::string &process_path, const std::string &map_path){
        m_simulations.resize(size);
        SetSimsAsyc(m_simulations.begin(), m_simulations.end(), net_address, port_start, process_path, map_path);
    }
    void StartSims()
    {
        StartSims(m_simulations.begin(), m_simulations.end());
    }
    // copy the state from main game and set the orders to simulations
    void CopyStateAndSendOrders(const ObservationInterface *ob, const std::vector<std::vector<Command>> &orders)
    {
        //if the orders' size is larger than the simulations' size, add some new simulations
        if (orders.size() > m_simulations.size())
        {
            // add some simulations and copy and send orders
            // start those sims
            int set_index = m_sol_sim_map.size();
            int diff_sz = orders.size() - m_simulations.size();
            // add, set and start the new sim one by one
            for (size_t i = 0; i < diff_sz; i++)
            {
                m_simulations.emplace_back(Simulation<void>());
                Simulator& sim = m_simulations.back().sim;
                sim.SetBaseSettings(m_port_end, m_process_path, m_map_path, 1);

                std::async(std::launch::async, [&]() -> void {
                    sim.LaunchStarcraft();
                    sim.StartGame();
                });

                m_port_end += 2;
                m_sol_sim_map[set_index++] = &m_simulations.back(); // don't forget to set the map
            }
            
            // std::list<Simulation<void>>::iterator add_begin = m_simulations.end()-1;
            // m_simulations.insert(m_simulations.end(), diff_sz, Simulation<void>());
            // std::list<Simulation<void>>::iterator add_end = m_simulations.end();
            // SetAndStartSims(add_begin, add_end, m_net_address, m_port_end, m_process_path, m_map_path);
            // int original_size = m_sol_sim_map.size();
            // int target_size = m_simulations.size();
            // m_sol_sim_map.resize(target_size);
            // int
            // for (size_t i = original_size; i < target_size; i++)
            // {
            //     m_sol_sim_map[i] = 
            // }
            
        }
        //copy and set state
        int sz = orders.size();
        std::list<Simulation<void>>::iterator it = m_simulations.begin();
        for (size_t i = 0; i < sz, it != m_simulations.end(); ++i, ++it)
        {
            (*it).sim.CopyAndSetState(ob);
            (*it).sim.SetOrders(orders[i]);
        }
    }
    // Run all the simulations for specific number of steps
    bool RunSims(int steps, const std::vector<DebugRenderer *> &debug_renderers = std::vector<DebugRenderer *>())
    {
        //todo check the debug renderer size and sims' size
        if(debug_renderers.size()<m_sol_sim_map.size()){
            throw("debug_renderers are fewer than simulations size@SimulatorPool::RunSims()");
        }
        //todo run all the sims synchronously
        size_t sz = m_sol_sim_map.size();
        for (size_t i = 0; i < sz; i++)
        {
            Simulation<void>& simulation = *m_sol_sim_map[i];
            simulation.result_holder = std::async(std::launch::async, &Simulator::Run, &(m_sol_sim_map[i]->sim), steps, debug_renderers[i]);
        }
        //todo if there is any thread get stuck, throw it away and create a new one
        // the duration I need to wait for the simulation result
        std::chrono::time_point<std::chrono::steady_clock> deadline = std::chrono::steady_clock::now() + m_wait_duration;
        for (size_t i = 0; i < sz; i++)
        {
            std::list<std::thread> thread_list;
            std::future_status result_status =  m_sol_sim_map[i]->result_holder.wait_until(deadline);
            switch (result_status)
            {
            case std::future_status::ready:
                //todo ok
                break;
            case std::future_status::timeout:
                //todo add a new simulation and reset the map... but how to handle the result?
                m_simulations.push_back(Simulation<void>());
                Simulation<void> &new_sim = m_simulations.back();
                m_sol_sim_map[i] = &new_sim;
                new_sim.sim.SetBaseSettings(m_port_end, m_process_path, m_map_path);
                m_port_end += 2;
                thread_list.push_back(std::thread{[&new_sim]()->void{
                    new_sim.sim.LaunchStarcraft();
                    new_sim.sim.StartGame();
                    }});
                break;
            default:
                throw("how can this async task be deffered?@SimulatorPool::RunSims()");
                break;
            }
            for (auto& item:thread_list)
            {
                item.join();
            }
        }
        return true;
    }
    // get the observation interface to get the result of the simulation
    const ObservationInterface *GetObservation(int i) const
    {
        return m_sol_sim_map[i]->sim.Observation();
    }

    ~SimulatorPool();

private:
    void SetAndStartSims(const std::list<Simulation<void>>::iterator &begin,
                         const std::list<Simulation<void>>::iterator &end,
                         const std::string &net_address, int port_start, const std::string &process_path, const std::string &map_path)
    {
        SetSimsAsyc(begin, end, net_address, port_start, process_path, map_path);
        StartSims(begin, end);
    }
    void SetSimsAsyc(const std::list<Simulation<void>>::iterator &begin,
                 const std::list<Simulation<void>>::iterator &end,
                 const std::string &net_address, int port_start, const std::string &process_path, const std::string &map_path)
    {
        for (auto it = begin; it != end; ++it)
        {
            Simulator &sim = (*it).sim;
            //sim.SetNetAddress(m_net_address);
            sim.SetPortStart(port_start);
            sim.SetProcessPath(process_path);
            sim.SetMapPath(map_path);
            sim.SetStepSize(1);
            port_start += 2;
        }
        if (port_start > m_port_end)
        {
            m_port_end = port_start;
        }
    }
    void StartSims(const std::list<Simulation<void>>::iterator &begin, const std::list<Simulation<void>>::iterator &end)
    {
        for (std::list<Simulation<void>>::iterator it = begin; it != end; ++it)
        {
            (*it).result_holder = std::async(std::launch::async, [&]() -> void {
                (*it).sim.LaunchStarcraft();
                (*it).sim.StartGame();
            });
        }
    }
    // settings to be stored
    std::string m_net_address = "0.0.0.0";
    int m_port_start;
    int m_port_end = m_port_start;
    std::string m_process_path;
    std::string m_map_path;
    std::chrono::milliseconds m_wait_duration{30000};

    // Simulation list to hold simulations and their thread holder
    std::list<Simulation<void>> m_simulations;
    // the solution-simulation map. Why don't I use map? Intger indices are enough.
    std::vector<Simulation<void>*> m_sol_sim_map;
};

} // namespace sc2

#endif
