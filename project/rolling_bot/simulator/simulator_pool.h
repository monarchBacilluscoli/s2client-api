#ifndef SIMULATOR_POOL_H
#define SIMULATOR_POOL_H

#include <future>
#include <chrono>
#include <string>
#include <vector>
#include <set>
#include <thread>
#include "simulator.h"

namespace sc2
{

using Population = std::vector<Solution<Command>>;

// I must lay aside all things about a thread, that is the thread result holder and simulator obj, so I'd better putting them into one class
template <typename T>
struct Simulation
{
    //Simulation() : sim(), result_holder(){};
    Simulator sim;
    // T is used to hold result type, but for now, void is enough
    std::future<T> result_holder;
    // useful?
    DebugRenderer *debug_renderer{nullptr};
};

class SimulatorPool
{
public:
    SimulatorPool() = default;
    SimulatorPool(SimulatorPool &rhs) = delete;
    SimulatorPool(int size, const std::string &net_address, int port_start, const std::string &process_path, const std::string &map_path);
    // if you use default constructor you must call this function to set the sims
    void SetSims(int size, const std::string &net_address, int port_start, const std::string &process_path, const std::string &map_path);
    void StartSimsAsync();
    // copy the state from main game and set the orders to simulations
    void CopyStateAndSendOrdersAsync(const ObservationInterface *ob, const std::vector<std::vector<Command>> &orders);
    // for easy use
    void CopyStateAndSendOrdersAsync(const ObservationInterface *ob, const Population &pop);
    // Run all the simulations for specific number of steps
    void RunSimsAsync(int steps, DebugRenderers &debug_renderers); // concurrency version
    void RunSimsAsync(int steps);
    void RunSimsOneByOne(int steps, DebugRenderers &debug_renderers); // single-thread version
    // get the observation interface to get the result of the simulation
    const ObservationInterface *GetObservation(int i) const
    {
        return m_sol_sim_map[i]->sim.Observation();
    }

    size_t size() { return m_sol_sim_map.size(); };

    // for simple use
    float GetTeamHealthLoss(int i, Unit::Alliance team) { return m_sol_sim_map[i]->sim.GetTeamHealthLoss(team); }

    ~SimulatorPool() = default;

private:
    // void SetAndStartSims(const std::list<Simulation<std::thread::id>>::iterator &begin,
    //                      const std::list<Simulation<std::thread::id>>::iterator &end,
    //                      const std::string &net_address, int port_start, const std::string &process_path, const std::string &map_path)
    // {
    //     SetSims(begin, end, net_address, port_start, process_path, map_path);
    //     StartSims(begin, end);
    // }
    // void SetSims(const std::list<Simulation<std::thread::id>>::iterator &begin,
    //              const std::list<Simulation<std::thread::id>>::iterator &end,
    //              const std::string &net_address, int port_start, const std::string &process_path, const std::string &map_path)
    // {
    //     for (auto it = begin; it != end; ++it)
    //     {
    //         Simulator &sim = (*it).sim;
    //         //sim.SetNetAddress(m_net_address);
    //         sim.SetPortStart(port_start);
    //         sim.SetProcessPath(process_path);
    //         sim.SetMapPath(map_path);
    //         sim.SetStepSize(1);
    //         port_start += 2;
    //     }
    //     if (port_start > m_port_end)
    //     {
    //         m_port_end = port_start;
    //     }
    // }
    // settings to be stored
    std::string m_net_address = "0.0.0.0";
    int m_port_start;
    int m_port_end = m_port_start;
    std::string m_process_path;
    std::string m_map_path;
    std::chrono::milliseconds m_wait_duration{10000};

    // observation interface to store the game state
    // used in RunSimsAsync() to recopy state to all the restarted sims
    const ObservationInterface *m_observation;

    // Simulation list to hold simulations and their thread holder
    std::list<Simulation<std::thread::id>> m_simulations;
    // the solution-simulation map. Why don't I use map? Intger indices are enough and suitble
    std::vector<Simulation<std::thread::id> *> m_sol_sim_map;
    // 
    std::set<int> m_timeout_index_set;
};

} // namespace sc2

#endif
