#ifndef SIMULATOR_POOL_H
#define SIMULATOR_POOL_H

#include "../../global_defines.h"

#include <future>
#include <chrono>
#include <string>
#include <vector>
#include <set>
#include <thread>
#include "simulator.h"

namespace sc2
{

    using Population = std::vector<RollingSolution<Command>>;

    // I must lay aside all things about a thread, that is the thread result holder and simulator obj, so I'd better putting them into one class
    template <typename T>
    struct Simulation
    {
        //Simulation() : sim(), result_holder(){};
        Simulator sim;
        // T is used to hold result type, but for now, void is enough
        std::future<T> result_holder;
#ifdef USE_GRAPHICS
        // useful?
        DebugRenderer *debug_renderer{nullptr};
#endif // USE_GRAPHICS
    };

    class SimulatorPool
    {
    public:
        SimulatorPool() = default;
        SimulatorPool(SimulatorPool &rhs) = delete;
        SimulatorPool(int size, const std::string &net_address, int port_start, const std::string &process_path, const std::string &map_filename);
        // if you use default constructor you must call this function to set the sims
        void SetSims(int size, const std::string &net_address, int port_start, const std::string &process_path, const std::string &map_filename);
        void StartSimsAsync();
        // copy the state from main game and set the orders to simulations
        void CopyStateAndSendOrdersAsync(const ObservationInterface *ob, const std::vector<std::vector<Command>> &orders);
        // for easy use
        void CopyStateAndSendOrdersAsync(const ObservationInterface *ob, const Population &pop);
        void CopyStateAsync(const ObservationInterface *ob, int sims_size);
        void SendOrders(const std::vector<Command>& my_order, const std::vector<Command>& rival_order, int sim_index); // or you can use the operator[] to set it
        // Run all the simulations for specific number of steps
        void RunSimsAsync(int steps);
        void RunSimsOneByOne(int steps);
#ifdef USE_GRAPHICS
        void RunSimsAsync(int steps, DebugRenderers &debug_renderers);    // concurrency version
        void RunSimsOneByOne(int steps, DebugRenderers &debug_renderers); // single-thread version
#endif                                                                    // USE_GRAPHICS
        Simulator &operator[](int i)
        {
            return m_sol_sim_map[i]->sim;
        }

        const Simulator &at(int i) const
        {
            return m_sol_sim_map.at(i)->sim;
        }

        // get the observation interface to get the result of the simulation
        const ObservationInterface *GetObservation(int i) const
        {
            return m_sol_sim_map[i]->sim.Observation();
        }

        size_t size() { return m_sol_sim_map.size(); };

        // for simple use
        float GetTeamHealthLoss(int i, Unit::Alliance team)
        {
            return m_sol_sim_map[i]->sim.GetTeamHealthLoss(team);
        }

        // methods for debug
        void SaveReplays(const std::string &folder, const std::string &file_name_prefix = "sim"); // save the replay up to now
        void SaveReplay(int sim_index, const std::string &path);

        ~SimulatorPool() = default;

    private:
        // settings to be stored
        std::string m_net_address = "0.0.0.0";
        int m_port_start;
        int m_port_end = m_port_start;
        std::string m_process_path;
        std::string m_map_path;
        std::chrono::milliseconds m_wait_duration{100000};

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
