#include "simulator_pool.h"
#include "sc2utils/sc2_manage_process.h"
namespace sc2
{
    using Population = std::vector<RollingSolution<Command>>;

    SimulatorPool::SimulatorPool(int size,
                                 const std::string &net_address,
                                 int port_start,
                                 const std::string &process_path,
                                 const std::string &map_path,
                                 int controlled_player_num) : m_simulations(size),
                                                              m_sol_sim_map(size),
                                                              m_net_address(net_address),
                                                              m_port_start(port_start),
                                                              m_port_end(port_start),
                                                              m_process_path(process_path),
                                                              m_map_path(map_path)
    {
        std::string sim_map_path = Simulator::GenerateSimMapPath(map_path);
        int i = 0;
        for (Simulation<std::thread::id> &simulation : m_simulations)
        {
            Simulator &sim = simulation.sim;
            //sim.SetNetAddress(m_net_address);
            sim.SetPortStart(port_start);
            sim.SetProcessPath(process_path);
            sim.SetMapPath(sim_map_path);
            sim.SetStepSize(1);
            sim.SetControlledPlayerNum(controlled_player_num);
            port_start += 10;                 // single player +2 is enough, but multiplayer sim needs -1,0,2,3,4,5 6 ports and 7 port nums.
            m_sol_sim_map[i++] = &simulation; // don't forget to set the map
        }
        m_port_end = port_start;
    };

    void SimulatorPool::SetSims(int size, const std::string &net_address, int port_start, const std::string &process_path, const std::string &map_path, int controlled_player_num)
    {
        m_simulations.resize(size);
        m_sol_sim_map.resize(size);
        std::string sim_map_path = Simulator::GenerateSimMapPath(map_path);
        int i = 0;
        for (Simulation<std::thread::id> &simulation : m_simulations)
        {
            Simulator &sim = simulation.sim;
            //sim.SetNetAddress(m_net_address);
            sim.SetPortStart(port_start);
            sim.SetProcessPath(process_path);
            sim.SetMapPath(sim_map_path);
            sim.SetStepSize(1);
            sim.SetControlledPlayerNum(controlled_player_num);
            port_start += 2;
            m_sol_sim_map[i++] = &simulation; // don't forget to set the map
        }
        if (port_start > m_port_end)
        {
            m_port_end = port_start;
        }
    }

    void SimulatorPool::StartSimsAsync(int batch_size)
    {
        //todo ensure I have set the map right. and then use the map to start all games
        int sz = m_sol_sim_map.size();
        for (int i = 0; i < sz; ++i)
        {
            // std::cout << i++ << std::endl;
            m_sol_sim_map[i]->result_holder = std::async(std::launch::async, [sim = m_sol_sim_map[i]] { //note sim is a pointer
                try
                {
                    sim->sim.LaunchStarcraft();
                }
                catch (const std::exception &e)
                {
                    std::cerr << e.what() << '\n';
                    std::vector<ProcessInfo> infos = sim->sim.GetProcessInfo();
                    for (int j = 0; j < 5; ++j)
                    {

                        if (IsProcessRunning(infos[0].process_id))
                        {
                            TerminateProcess(infos[0].process_id);
                        }
                        if (infos.size() > 1 && IsProcessRunning(infos[1].process_id))
                        {
                            TerminateProcess(infos[1].process_id);
                        }
                        SleepFor(500);
                    }
                    sim->sim.ClearOldProcessInfo();
                    sim->sim.LaunchStarcraft();
                }
                while (!sim->sim.StartGame())
                {
                    sim->sim.ClearOldProcessInfo();
                    sim->sim.ResetExecutors();
                    sim->sim.LaunchStarcraft(); // 一般都是Launch也没Launch对，保险起见重新launch一遍
                }

                return std::this_thread::get_id();
            });
            if (i != 0 && ((i + 1) % batch_size == 0 || i == sz))
            {
                for (int j = i - batch_size + 1; j <= i; ++j)
                {
                    m_sol_sim_map[j]->result_holder.wait();
                }
            }
        }
        // for (Simulation<std::thread::id> *simulation : m_sol_sim_map)
        // {
        //     simulation->result_holder.wait();
        // }
        return;
    }

    void SimulatorPool::CopyStateAndSendOrdersAsync(const ObservationInterface *ob, const std::vector<std::vector<Command>> &orders)
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
                m_simulations.emplace_back(Simulation<std::thread::id>());
                Simulator &sim = m_simulations.back().sim;
                sim.SetBaseSettings(m_port_end, m_process_path, m_map_path, 1);

                m_simulations.back().result_holder = std::async(std::launch::async, [&] {
                    sim.LaunchStarcraft();
                    sim.StartGame();
                    return std::this_thread::get_id();
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
                return std::this_thread::get_id();
            });
        }
        for (size_t i = 0; i < sz; ++i)
        {
            m_sol_sim_map[i]->result_holder.wait(); // must ensure all the thread finished, then you can do the next things
        }
    }

    void SimulatorPool::CopyStateAndSendOrdersAsync(const ObservationInterface *ob, const Population &pop)
    {
        size_t pop_size = pop.size();
        m_observation = ob;
        //if the orders' size is larger than the simulations' size, add some new simulations
        if (pop_size > m_simulations.size())
        {
            int diff_sz = pop.size() - m_simulations.size();
            int set_index = m_sol_sim_map.size();
            m_sol_sim_map.resize(pop_size);
            for (int i = set_index; i < pop_size; ++i)
            {
                m_simulations.emplace_back(Simulation<std::thread::id>());
                Simulator &sim = m_simulations.back().sim;
                sim.SetBaseSettings(m_port_end, m_process_path, m_map_path, 1);

                m_simulations.back().result_holder = std::async(std::launch::async, [&] {
                    sim.LaunchStarcraft();
                    sim.StartGame();
                    return std::this_thread::get_id();
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
        std::vector<std::future<void>> copy_holders(pop_size); // I just need temporary holders to hold all threads. And when they are destryed, they will wait for the return of those threads
        for (size_t i = 0; i < pop_size; ++i)
        {
            copy_holders[i] = std::async([&sim = m_sol_sim_map[i]->sim, &ob, &order = pop[i].variable]() -> void {
                sim.CopyAndSetState(ob);
                sim.SetOrders(order);
            });
        }
        std::chrono::time_point<std::chrono::steady_clock> deadline = std::chrono::steady_clock::now() + m_wait_duration;
        for (size_t i = 0; i < pop_size; ++i)
        {
            std::future_status result_status = copy_holders[i].wait_until(deadline);
            switch (result_status)
            {
            case std::future_status::ready:
            {
                // ok
                break;
            }
            case std::future_status::timeout:
            { // add a new simulation and reset the map... but how to handle the result?
                std::cout << "sim " << i << " timeout in " << __FUNCTION__ << std::endl;
                break;
            }
            default:
            {
                throw("how can this async task be deffered?@" + std::string(__FUNCTION__));
                break;
            }
            }
        }
    }

    void SimulatorPool::CopyStateAsync(const ObservationInterface *ob, int sims_size)
    {
        m_observation = ob;
        // launch some new simulators
        if (sims_size > m_simulations.size())
        {
            int diff_sz = sims_size - m_simulations.size();
            int set_index = m_sol_sim_map.size();
            m_sol_sim_map.resize(sims_size);
            for (int i = set_index; i < sims_size; ++i)
            {
                m_simulations.emplace_back(Simulation<std::thread::id>());
                Simulator &sim = m_simulations.back().sim;
                sim.SetBaseSettings(m_port_end, m_process_path, m_map_path, 1);

                m_simulations.back().result_holder = std::async(std::launch::async, [&] {
                    sim.LaunchStarcraft();
                    sim.StartGame();
                    return std::this_thread::get_id();
                });

                m_port_end += 2;
                m_sol_sim_map[i] = &m_simulations.back(); // don't forget to set the map
            }
            for (size_t i = set_index; i < sims_size; i++)
            {
                m_sol_sim_map[i]->result_holder.wait();
            }
        }
        // copy state ssyncly
        std::vector<std::future<void>> copy_holders(sims_size); // I just need temporary holders to hold all threads. And when they are destroyed, they will wait for the return of those threads
        for (size_t i = 0; i < sims_size; ++i)
        {
            copy_holders[i] = std::async([&sim = m_sol_sim_map[i]->sim, &ob]() -> void {
                sim.CopyAndSetState(ob);
            });
        }
        std::chrono::time_point<std::chrono::steady_clock> deadline = std::chrono::steady_clock::now() + m_wait_duration;
        for (size_t i = 0; i < sims_size; ++i)
        {
            std::future_status result_status = copy_holders[i].wait_until(deadline);
            switch (result_status)
            {
            case std::future_status::ready:
            {
                // ok
                break;
            }
            case std::future_status::timeout:
            { // add a new simulation and reset the map... but how to handle the result?
                std::cout << "sim " << i << " timeout in " << __FUNCTION__ << std::endl;
                break;
            }
            default:
            {
                throw("how can this async task be deffered?@" + std::string(__FUNCTION__));
                break;
            }
            }
        }
        return;
    }

    void SimulatorPool::SendOrders(const std::vector<Command> &my_order, const std::vector<Command> &rival_order, int sim_index)
    {
        m_sol_sim_map[sim_index]->sim.SetOrders(my_order);
        return;
    }

#ifdef USE_GRAPHICS
    void SimulatorPool::RunSimsAsync(int steps, DebugRenderers &debug_renderers)
    {
        // check the debug renderer size and sims' size

        if (debug_renderers.size() < m_sol_sim_map.size())
        {
            throw("debug_renderers are fewer than simulations size@" + std::string(__FUNCTION__));
        }

        // run all the sims synchronously
        size_t sz = m_sol_sim_map.size();
        for (size_t i = 0; i < sz; i++)
        {
            Simulation<std::thread::id> &simulation = *m_sol_sim_map[i];
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
                std::cout << "sim " << i << " timeout in " << __FUNCTION__ << std::endl;
                m_timeout_index_set.insert(i);

                /* restart a new simulation and add it into my simulation pool. 
            since the old result holder has been blocked (don't care whatever caused it), if I can not solve the problem in that thread, I'd better not changing the current resources it is using, and add a new here.
            */
                /* But unfortunely, I can not reset the debug_rederer (and for now it is the problem-causer!) outside it and I have to use it. So I must overturn my opinion above instantly.
            I need to reconstruct it from inside!
            ...no use, the reconstruct function gets blocked...
            */
                //reset the simulation
                Simulation<std::thread::id> sim = Simulation<std::thread::id>();
                m_simulations.emplace_back();
                Simulation<std::thread::id> &new_sim = m_simulations.back();
                new_sim.sim.SetBaseSettings(m_port_end, m_process_path, Simulator::GenerateSimMapPath(m_map_path));
                m_port_end += 2;
                new_sim.sim.LaunchStarcraft();
                new_sim.sim.StartGame();
                new_sim.sim.CopyAndSetState(m_observation);
                new_sim.sim.SetOrders(m_sol_sim_map[i]->sim.GetOriginalOrders()); //! this is not the source of the problem

                m_sol_sim_map[i] = &new_sim;
                break;
            }
            default:
            {
                throw("how can this async task be deffered?@" + std::string(__FUNCTION__));
                break;
            }
            }
            //? Do I need to run it?
        }
    }

    void SimulatorPool::RunSimsOneByOne(int steps, DebugRenderers &debug_renderers)
    {
        if (debug_renderers.size() < m_sol_sim_map.size())
        {
            throw("debug_renderers are fewer than simulations size@" + std::string(__FUNCTION__));
        }
        size_t sz = m_sol_sim_map.size();
        for (size_t i = 0; i < sz; i++)
        {
            Simulation<std::thread::id> &simulation = *m_sol_sim_map[i];
            simulation.sim.Run(steps, m_timeout_index_set.find(i) != m_timeout_index_set.end() ? nullptr : &debug_renderers[i]);
        }
        // todo there is no problem handling code
    }
#endif // USE_GRAPHICS

    void SimulatorPool::RunSimsOneByOne(int steps)
    {
        size_t sz = m_sol_sim_map.size();
        for (size_t i = 0; i < sz; i++)
        {
            Simulation<std::thread::id> &simulation = *m_sol_sim_map[i];
            simulation.sim.Run(steps);
        }
    }

    void SimulatorPool::RunSimsAsync(int steps)
    {
        // run all the sims synchronously
        size_t sz = m_sol_sim_map.size();
        for (size_t i = 0; i < sz; i++)
        {
            Simulation<std::thread::id> &simulation = *m_sol_sim_map[i];
            simulation.result_holder = std::async(std::launch::async, &Simulator::Run, &(m_sol_sim_map[i]->sim), steps
#ifdef USE_GRAPHICS
                                                  ,
                                                  nullptr
#endif // USE_GRAPHICS
            );
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

                Simulation<std::thread::id> sim = Simulation<std::thread::id>();
                m_simulations.emplace_back();
                Simulation<std::thread::id> &new_sim = m_simulations.back();
                new_sim.sim.SetBaseSettings(m_port_end, m_process_path, Simulator::GenerateSimMapPath(m_map_path));
                m_port_end += 2;
                new_sim.sim.LaunchStarcraft();
                new_sim.sim.StartGame();
                new_sim.sim.CopyAndSetState(m_observation);
                new_sim.sim.SetOrders(m_sol_sim_map[i]->sim.GetOriginalOrders()); //! this is not the source of the problem
                m_sol_sim_map[i] = &new_sim;
                break;
            }
            default:
            {
                throw("how can this async task be deffered?@" + std::string(__FUNCTION__));
                break;
            }
            }
            //? Do I need to run it?
        }
    }

    void SimulatorPool::SaveReplays(const std::string &folder, const std::string &file_name_prefix)
    {

        if (folder.back() == '/')
        {
            for (size_t i = 0; i < m_sol_sim_map.size(); ++i)
            {
                SaveReplay(i, folder + file_name_prefix + std::to_string(i) + ".SC2Replay");
            }
        }
        else
        {
            for (size_t i = 0; i < m_sol_sim_map.size(); ++i)
            {
                SaveReplay(i, folder + '/' + file_name_prefix + std::to_string(i) + ".SC2Replay");
            }
        }
    }

    void SimulatorPool::SaveReplay(int sim_index, const std::string &path)
    {
        if (!m_sol_sim_map[sim_index]->sim.Control()->SaveReplay(path))
        {
            throw(std::string("no folder here, please create folder first@") + __FUNCTION__);
        }
    }

} // namespace sc2