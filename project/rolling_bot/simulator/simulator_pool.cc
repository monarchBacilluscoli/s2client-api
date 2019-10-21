#include "simulator_pool.h"

// #define __FUNCTION__

namespace sc2
{
using Population = std::vector<Solution<Command>>;

SimulatorPool::SimulatorPool(int size, const std::string &net_address, int port_start, const std::string &process_path, const std::string &map_path) : m_simulations(size),
                                                                                                                                                       m_sol_sim_map(size),
                                                                                                                                                       m_net_address(net_address),
                                                                                                                                                       m_port_start(port_start),
                                                                                                                                                       m_port_end(port_start),
                                                                                                                                                       m_process_path(process_path),
                                                                                                                                                       m_map_path(map_path)
{
    int i = 0;
    for (Simulation<void> &simulation : m_simulations)
    {
        Simulator &sim = simulation.sim;
        //sim.SetNetAddress(m_net_address);
        sim.SetPortStart(port_start);
        sim.SetProcessPath(process_path);
        sim.SetMapPath(map_path);
        sim.SetStepSize(1);
        port_start += 2;
        m_sol_sim_map[i++] = &simulation; // don't forget to set the map
    }
    m_port_end = port_start;
};

void SimulatorPool::SetSims(int size, const std::string &net_address, int port_start, const std::string &process_path, const std::string &map_path)
{
    m_simulations.resize(size);
    m_sol_sim_map.resize(size);
    int i = 0;
    for (Simulation<void> &simulation : m_simulations)
    {
        Simulator &sim = simulation.sim;
        //sim.SetNetAddress(m_net_address);
        sim.SetPortStart(port_start);
        sim.SetProcessPath(process_path);
        sim.SetMapPath(map_path);
        sim.SetStepSize(1);
        port_start += 2;
        m_sol_sim_map[i++] = &simulation; // don't forget to set the map
    }
    if (port_start > m_port_end)
    {
        m_port_end = port_start;
    }
}

void SimulatorPool::StartSimsAsync()
{
    //todo ensure I have set the map right. and then use the map to start all games
    int i = 0;
    for (Simulation<void> *simulation : m_sol_sim_map)
    {
        // std::cout << i++ << std::endl;
        simulation->result_holder = std::async(std::launch::async, [sim = simulation]() -> void {
            sim->sim.LaunchStarcraft();
            sim->sim.StartGame();
        });
    }
    for (Simulation<void> *simulation : m_sol_sim_map)
    {
        simulation->result_holder.wait();
    }
}

void SimulatorPool::CopyStateAndSendOrders(const ObservationInterface *ob, const std::vector<std::vector<Command>> &orders)
{
    m_observation = ob;
    //if the orders' size is larger than the simulations' size, add some new simulations
    int sz = orders.size();
    if (sz > m_simulations.size())
    {
        int target_size = orders.size();
        int set_index = m_sol_sim_map.size();
        m_sol_sim_map.resize(target_size);
        int diff_sz = target_size - m_simulations.size();
        // add, set and start the new sims one by one
        for (size_t i = set_index; i < target_size; ++i)
        {
            m_simulations.emplace_back(Simulation<void>());
            Simulator &sim = m_simulations.back().sim;
            sim.SetBaseSettings(m_port_end, m_process_path, m_map_path, 1);

            m_simulations.back().result_holder = std::async(std::launch::async, [&]() -> void {
                sim.LaunchStarcraft();
                sim.StartGame();
            });

            m_port_end += 2;
            m_sol_sim_map[i] = &m_simulations.back(); // don't forget to set the map
        }
        for (size_t i = set_index; i < target_size; ++i)
        {
            m_sol_sim_map[i]->result_holder.wait();
        }
    }
    for (size_t i = 0; i < sz; ++i)
    {
        m_sol_sim_map[i]->result_holder = std::async([&sim = m_sol_sim_map[i]->sim, &ob, &order = orders[i]] {
            sim.CopyAndSetState(ob);
            sim.SetOrders(order);
        });
    }
    for (size_t i = 0; i < sz; ++i)
    {
        m_sol_sim_map[i]->result_holder.wait(); // must ensure all the thread finished, then you can do the next things
    }
}

void SimulatorPool::CopyStateAndSendOrders(const ObservationInterface *ob, const Population &pop)
{
    size_t pop_size = pop.size();
    m_observation = ob;
    //if the orders' size is larger than the simulations' size, add some new simulations
    if (pop_size > m_simulations.size())
    {
        int diff_sz = pop.size() - m_simulations.size();
        int set_index = m_sol_sim_map.size();
        m_sol_sim_map.resize(pop_size);
        // add, set and start the new sims one by one
        for (size_t i = set_index; i < pop_size; i++)
        {
            m_simulations.emplace_back(Simulation<void>());
            Simulator &sim = m_simulations.back().sim;
            sim.SetBaseSettings(m_port_end, m_process_path, m_map_path, 1);

            m_simulations.back().result_holder = std::async(std::launch::async, [&]() -> void {
                sim.LaunchStarcraft();
                sim.StartGame();
            });

            m_port_end += 2;
            m_sol_sim_map[i] = &m_simulations.back(); // don't forget to set the map
        }
        for (size_t i = set_index; i < pop_size; i++)
        {
            m_sol_sim_map[i]->result_holder.wait();
        }
    }
    //copy and set state asyncly
    for (size_t i = 0; i < pop_size; ++i)
    {
        m_sol_sim_map[i]->result_holder = std::async([&sim = m_sol_sim_map[i]->sim, &ob, &order = pop[i].variable]() -> void {
            sim.CopyAndSetState(ob);
            sim.SetOrders(order);
        });
    }
    for (size_t i = 0; i < pop_size; ++i)
    {
        m_sol_sim_map[i]->result_holder.wait(); // must ensure all the thread finished, then you can do the next things
        // std::cout << i << "copied!" << std::endl;
    }
}

void SimulatorPool::RunSimsAsync(int steps, DebugRenderers &debug_renderers)
{
    //! for test
    if(!m_timeout_index_set.empty()){
        RunSimsOneByOne(steps, debug_renderers);
        return;
    }
    //! for test/
    // check the debug renderer size and sims' size
    if (debug_renderers.size() < m_sol_sim_map.size())
    {
        throw("debug_renderers are fewer than simulations size@SimulatorPool::" + std::string(__FUNCTION__));
    }
    // run all the sims synchronously
    size_t sz = m_sol_sim_map.size();
    for (size_t i = 0; i < sz; i++)
    {
        Simulation<void> &simulation = *m_sol_sim_map[i];
        simulation.result_holder = std::async(std::launch::async, &Simulator::Run, &(m_sol_sim_map[i]->sim), steps, (m_timeout_index_set.find(i) != m_timeout_index_set.end()) ? nullptr : &debug_renderers[i]);
    }
    // if there is any thread get stuck (timeout), throw it away and create a new one
    std::chrono::time_point<std::chrono::steady_clock> deadline = std::chrono::steady_clock::now() + m_wait_duration;
    for (size_t i = 0; i < sz; i++)
    {
        std::list<std::thread> thread_list;
        std::future_status result_status = m_sol_sim_map[i]->result_holder.wait_until(deadline);
        switch (result_status)
        {
        case std::future_status::ready:
        {
            // ok
            break;
        }
        case std::future_status::timeout:
        { // add a new simulation and reset the map... but how to handle the result?
            std::cout << "sim " << i << " timeout..." << std::endl;
            m_timeout_index_set.insert(i);
            // //! for test
            // is_timeout = true;
            // //! for test/
            // Simulation<void> sim = Simulation<void>();
            // m_simulations.emplace_back();
            // Simulation<void> &new_sim = m_simulations.back();
            // new_sim.sim.SetBaseSettings(m_port_end, m_process_path, m_map_path);
            // m_port_end += 2;
            // // thread_list.push_back(std::thread{[&new_sim, &observation = m_observation, orders = m_sol_sim_map[i]->sim.GetOrders()]() -> void {
            // new_sim.sim.LaunchStarcraft();
            // new_sim.sim.StartGame();
            // new_sim.sim.CopyAndSetState(m_observation);
            // new_sim.sim.SetOrders(m_sol_sim_map[i]->sim.GetOriginalOrders()); //! this is not the source of the problem
            // // }});
            // m_sol_sim_map[i] = &new_sim;
            // destroy and reconstruct the renderer obj, since it seems the problem of the timeout
            // debug_renderers.ReconstructAll();

            //? test if I set the timeout renderer pointer to nullptr...

            //? test if I set the timeout renderer pointer to nullptr.../
            break;
        }
        default:
        {
            throw("how can this async task be deffered?@SimulatorPool::RunSims()");
            break;
        }
        }
        // for (auto &item : thread_list)
        // {
        //     item.join();
        // }
        //? Do I need to run it?
    }
}

void SimulatorPool::RunSimsOneByOne(int steps, DebugRenderers &debug_renderers)
{
    if (debug_renderers.size() < m_sol_sim_map.size())
    {
        throw("debug_renderers are fewer than simulations size@SimulatorPool::" + std::string(__FUNCTION__));
    }
    size_t sz = m_sol_sim_map.size();
    for (size_t i = 0; i < sz; i++)
    {
        Simulation<void> &simulation = *m_sol_sim_map[i];
        simulation.sim.Run(steps, &debug_renderers[i]);
    }
    // todo there is no problem handling code
}

} // namespace sc2