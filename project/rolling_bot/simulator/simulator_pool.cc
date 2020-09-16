#include "simulator_pool.h"
#include "sc2utils/sc2_manage_process.h"
#include <sc2utils/port_checker.h>
#include <sys/wait.h>
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
        SetSims(size, net_address, port_start, process_path, map_path, controlled_player_num);
    };

    void SimulatorPool::SetSims(int size, const std::string &net_address, int port_start, const std::string &process_path, const std::string &map_path, int controlled_player_num)
    {
        if (port_start + size * 20 > std::numeric_limits<uint16_t>::max() || port_start < 61000)
        {
            throw("Suggest you seting the port_start between 61000(which is bigger than what you can find in /proc/sys/net/ipv4/ip_local_port_range) and 65535@" + std::string(__FUNCTION__));
        }
        m_simulations.resize(size);
        m_sol_sim_map.resize(size);
        std::string sim_map_path = Simulator::GenerateSimMapPath(map_path);
        int i = 0;
        for (Simulation<std::thread::id> &simulation : m_simulations)
        {
            Simulator &sim = simulation.sim;
            sim.SetPortStart(port_start);
            sim.SetProcessPath(process_path);
            sim.SetMapPath(sim_map_path);
            sim.SetStepSize(1);
            sim.SetControlledPlayerNum(controlled_player_num);
            port_start += 20;               // a value set by experience
            m_sol_sim_map[i] = &simulation; // don't forget to set the map
            ++i;
        }
        if (port_start > m_port_end)
        {
            m_port_end = port_start;
        }
        return;
    }

    void SimulatorPool::StartSimsAsync(int batch_size) //? too many try...catch, ugly.
    {
        std::mutex exception_mtx;
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        int sz = m_sol_sim_map.size();
        if (batch_size > sz)
        {
            batch_size = 0; // 0 means all the processes are in the same batch
        }
        for (int i = 0; i < sz; ++i)
        {
            m_sol_sim_map[i]->result_holder =
                std::async(std::launch::async,
                           [&, i] { //note sim is a pointer
                               PortChecker pc;
                               uint16_t new_port_start = pc.GetContinuousPortsFromPort(m_sol_sim_map[i]->sim.GetPortStart(), 7);
                               //    SleepFor(500);
                               m_sol_sim_map[i]->sim.SetPortStart(new_port_start + 1); //this is wierd,but the implementation is wierd, too.
                               try
                               {
                                   m_sol_sim_map[i]->sim.LaunchStarcraft();
                               }
                               catch (const std::exception &e) // if there is any exception during launching, kill then relaunch the game
                               {
                                   m_sol_sim_map[i]->sim.LeaveGame();
                                   std::lock_guard<std::mutex> lg(exception_mtx);
                                   std::cerr << "launch unsuccessful" << e.what() << "port: " << m_sol_sim_map[i]->sim.GetPortStart() << "@" + std::string(__FUNCTION__) << std::endl;
                                   // 有任何问题，先杀掉原始程序
                                   std::vector<ProcessInfo> infos = m_sol_sim_map[i]->sim.GetProcessInfo();
                                   for (int k = 0; k < 3; ++k)
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
                                   int status;
                                   ::waitpid(infos[0].process_id, &status, WUNTRACED | WCONTINUED); // kill the zombie ps
                                   if (infos.size() > 1)
                                   {
                                       ::waitpid(infos[1].process_id, &status, WUNTRACED | WCONTINUED);
                                   }
                                   SleepFor(1000);
                                   m_sol_sim_map[i]->sim.ClearOldProcessInfo();
                                   m_sol_sim_map[i]->sim.ResetExecutors();
                                   uint16_t new_port_start = pc.GetContinuousPortsFromPort(m_sol_sim_map[i]->sim.GetPortStart(), 7);
                                   m_sol_sim_map[i]->sim.SetPortStart(new_port_start + 1);
                                   m_sol_sim_map[i]->sim.LaunchStarcraft();
                                   SleepFor(1000);
                               }
                               try
                               {
                                   while (!m_sol_sim_map[i]->sim.StartGame()) // if the game start unsuccessfully, kill&relaunch&restart.
                                   {
                                       m_sol_sim_map[i]->sim.LeaveGame();
                                       std::lock_guard<std::mutex> lg(exception_mtx);
                                       std::vector<ProcessInfo> infos = m_sol_sim_map[i]->sim.GetProcessInfo();
                                       for (int i = 0; i < 3; ++i)
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
                                       int status;
                                       ::waitpid(infos[0].process_id, &status, WUNTRACED | WCONTINUED); // kill the zombie ps
                                       if (infos.size() > 1)
                                       {
                                           ::waitpid(infos[1].process_id, &status, WUNTRACED | WCONTINUED);
                                       }
                                       SleepFor(1000);

                                       m_sol_sim_map[i]->sim.ClearOldProcessInfo();
                                       m_sol_sim_map[i]->sim.ResetExecutors();
                                       uint16_t new_port_start = pc.GetContinuousPortsFromPort(m_sol_sim_map[i]->sim.GetPortStart(), 7);
                                       m_sol_sim_map[i]->sim.SetPortStart(new_port_start + 1);
                                       m_sol_sim_map[i]->sim.LaunchStarcraft();
                                       SleepFor(1000);
                                   }
                               }
                               catch (const std::exception &e)
                               {
                                   m_sol_sim_map[i]->sim.LeaveGame();
                                   std::lock_guard<std::mutex> lg(exception_mtx);
                                   std::vector<ProcessInfo> infos = m_sol_sim_map[i]->sim.GetProcessInfo();
                                   for (int k = 0; k < 3; ++k)
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
                                   int status;
                                   ::waitpid(infos[0].process_id, &status, WUNTRACED | WCONTINUED); // kill the zombie ps
                                   if (infos.size() > 1)
                                   {
                                       ::waitpid(infos[1].process_id, &status, WUNTRACED | WCONTINUED);
                                   }
                                   SleepFor(1000);
                                   m_sol_sim_map[i]->sim.ClearOldProcessInfo();
                                   m_sol_sim_map[i]->sim.ResetExecutors();
                                   uint16_t new_port_start = pc.GetContinuousPortsFromPort(m_sol_sim_map[i]->sim.GetPortStart(), 7);
                                   m_sol_sim_map[i]->sim.SetPortStart(new_port_start + 1);
                                   m_sol_sim_map[i]->sim.LaunchStarcraft();
                                   SleepFor(1000);
                                   if (!m_sol_sim_map[i]->sim.StartGame())
                                   {
                                       std::cout << "还是没成功(err) port：" << new_port_start << std::endl;
                                   }
                               }
                               return std::this_thread::get_id();
                           });
            std::cout << batch_size << std::endl;
            std::cout << (i + 1) % batch_size << std::endl;
            if (batch_size != 0 && ((i + 1) % batch_size == 0 || i >= sz - 1))
            {
                int j = (i >= sz - 1 ? batch_size * (sz % batch_size == 0 ? sz / batch_size - 1 : sz / batch_size) : i - batch_size + 1);
                for (; j <= i; ++j)
                {
                    m_sol_sim_map[j]->result_holder.wait();
                }
                std::cout << "start batch until sim " << i << " finished!" << std::endl;
            }
        }
        if (batch_size == 0)
        {
            for (int i = 0; i < sz; ++i)
            {
                m_sol_sim_map[i]->result_holder.wait();
            }
        }
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::chrono::steady_clock::duration d = end - start;
        std::cout << "start time used: " << std::chrono::duration_cast<std::chrono::milliseconds>(d).count() << std::endl;
        //! update 一阵子看看效果
        // RunSimsAsync(100, 0);
        // for (size_t i = 0; i < sz; ++i)
        // {
        //     std::cout << m_sol_sim_map[i]->sim.GetObservations().front()->GetGameLoop() << '\t';
        // }
        // std::cout << std::endl;

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

                //!这一段绝对有问题。
                m_simulations.back().result_holder = std::async(std::launch::async, [&, i] {
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
            m_sol_sim_map[i]->result_holder = std::async([&, i] {
                m_sol_sim_map[i]->sim.CopyAndSetState(ob);
                m_sol_sim_map[i]->sim.SetOrders(orders[i]);
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

                //! 这一段绝对有问题
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
            copy_holders[i] = std::async([&, i]() -> void {
                m_sol_sim_map[i]->sim.CopyAndSetState(ob);
                m_sol_sim_map[i]->sim.SetOrders(pop[i].variable);
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

                //! 这一段绝对有问题
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
        // copy state asyncly
        std::vector<std::future<void>> copy_holders(sims_size); // I just need temporary holders to hold all threads. And when they are destroyed, they will wait for the return of those threads
        for (size_t i = 0; i < sims_size; ++i)
        {
            copy_holders[i] = std::async([&, i]() -> void {
                m_sol_sim_map[i]->sim.CopyAndSetState(ob);
            });
        }

        std::chrono::time_point<std::chrono::steady_clock> deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1000);
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
        m_sol_sim_map[sim_index]->sim.SetOrders(my_order, rival_order);
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

    void SimulatorPool::RunSimsAsync(int steps, int batch_size)
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
            if (batch_size != 0 && (i != 0 && ((i + 1) % batch_size == 0 || i >= sz - 1)))
            {
                std::chrono::time_point<std::chrono::steady_clock> deadline = std::chrono::steady_clock::now() + m_wait_duration;
                int j = batch_size * static_cast<int>(i / batch_size);
                for (; j <= i; ++j) // 似乎不太对
                {
                    std::future_status result_status = m_sol_sim_map[j]->result_holder.wait_until(deadline);
                    switch (result_status)
                    {
                    case std::future_status::ready:
                    {
                        break;
                    }
                    case std::future_status::timeout:
                    { //todo add a new simulation and reset the map... but how to handle the result?
                        std::cout << "sim " << i << " timeout..." << std::endl;
                        break;
                    }
                    default:
                    {
                        throw("how can this async task be deffered?@" + std::string(__FUNCTION__));
                        break;
                    }
                    }
                }
                std::cout << "update batch until sim " << i << " finished!" << std::endl;
            }
        }
        // if there is any thread get stuck (timeout), throw it away and create a new one
        if (batch_size == 0)
        {
            std::chrono::time_point<std::chrono::steady_clock> deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1000); //! modified for test
            for (size_t i = 0; i < sz; ++i)
            {
                std::list<std::thread> thread_list;
                std::future_status result_status = m_sol_sim_map[i]->result_holder.wait_until(deadline);
                switch (result_status)
                {
                case std::future_status::ready:
                {
                    // ok
                    continue;
                }
                case std::future_status::timeout:
                { // add a new simulation and reset the map... but how to handle the result?
                    std::cout << "sim " << i << " timeout..." << std::endl;
                    // m_timeout_index_set.insert(i);

                    // Simulation<std::thread::id> sim = Simulation<std::thread::id>();
                    // m_simulations.emplace_back();
                    // Simulation<std::thread::id> &new_sim = m_simulations.back();
                    // new_sim.sim.SetBaseSettings(m_port_end, m_process_path, Simulator::GenerateSimMapPath(m_map_path));
                    // m_port_end += 2;
                    // new_sim.sim.LaunchStarcraft();
                    // new_sim.sim.StartGame();
                    // new_sim.sim.CopyAndSetState(m_observation);
                    // new_sim.sim.SetOrders(m_sol_sim_map[i]->sim.GetOriginalOrders()); //! this is not the source of the problem
                    // m_sol_sim_map[i] = &new_sim;
                    continue;
                }
                default:
                {
                    throw("how can this async task be deffered?@" + std::string(__FUNCTION__));
                    continue;
                }
                }
            }
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