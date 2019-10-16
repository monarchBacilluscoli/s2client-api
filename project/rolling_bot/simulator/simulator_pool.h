#ifndef SIMULATOR_POOL_H
#define SIMULATOR_POOL_H

#include <future>
#include <chrono>
#include <string>
#include <vector>
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
                                                                                                                                            m_net_address(net_address),
                                                                                                                                            m_port_start(port_start),
                                                                                                                                            m_process_path(process_path),
                                                                                                                                            m_map_path(map_path)
    {
        for (Simulation<void> &simulation : m_simulations)
        {
            Simulator &sim = simulation.sim;
            //sim.SetNetAddress(m_net_address);
            sim.SetPortStart(port_start);
            sim.SetProcessPath(process_path);
            sim.SetMapPath(map_path);
            sim.SetStepSize(1);
            port_start += 2;
        }
        m_port_end = port_start;
    }
    // if you use default constructor you must call this function to set the sims
    void SetSims(int size, const std::string &net_address, int port_start, const std::string &process_path, const std::string &map_path){
        m_simulations.resize(size);
        SetSims(m_simulations.begin(), m_simulations.end(), net_address, port_start, process_path, map_path);
    }
    void StartSims()
    {
        StartSims(m_simulations.begin(), m_simulations.end());
    }
    // copy the state from main game and set the orders to simulations
    void CopyStateAndSendOrders(const ObservationInterface *ob, const std::vector<std::vector<Command>> &orders)
    {
        //todo check the size
        if (orders.size() > m_simulations.size())
        {
            // add some simulations and copy and send orders
            // start those sims
            int diff_sz = orders.size() - m_simulations.size();
            std::list<Simulation<void>>::iterator add_begin = m_simulations.end();
            m_simulations.insert(m_simulations.end(), diff_sz, Simulation<void>());
            std::list<Simulation<void>>::iterator it = m_simulations.end();
            SetAndStartSims(add_begin, m_simulations.end(), m_net_address, m_port_end, m_process_path, m_map_path);
        }
        //todo copy and set state
        int sz = m_simulations.size();
        for (size_t i = 0; i < sz; ++i)
        {
            m_simulations[i].sim.CopyAndSetState(ob);
            m_simulations[i].sim.SetOrders(orders[i]);
        }
    }
    // Run all the simulations for specific number of steps
    bool RunSims(int steps, const std::vector<DebugRenderer *> &debug_renderers(std::vector<DebugRenderer *>()))
    {
        //todo check the debug renderer size and sims' size

        //todo run all the sims synchronously
        //todo if there is any thread get stuck, throw it away and create a new one

        //todo or return ture
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
        SetSims(begin, end, net_address, port_start, process_path, map_path);
        StartSims(begin, end);
    }
    void SetSims(const std::list<Simulation<void>>::iterator &begin,
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
    void CopyStateAndSendOrders(const std::list<Simulation<void>>::iterator &begin, const std::list<Simulation<void>>::iterator &end, const ObservationInterface *ob, const std::vector<std::vector<Command>> &orders){
        for (auto it = begin; it != end; ++it)
        {
            (*it)
        }
        
        
    }
    // settings to be stored
    std::string m_net_address = "0.0.0.0";
    int m_port_start;
    int m_port_end = m_port_start;
    std::string m_process_path;
    std::string m_map_path;

    // Simulation list to hold simulations and their thread holder
    std::list<Simulation<void>> m_simulations;
    // the solution-simulation map. Why don't I use map? Intger indices are enough.
    std::vector<Simulation<void>*> m_sol_sim_map;
};

} // namespace sc2

#endif
