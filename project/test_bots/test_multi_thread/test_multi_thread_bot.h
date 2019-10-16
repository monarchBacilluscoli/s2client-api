//! this bot is used to test multi-=thread problem in my project
#ifndef TEST_MULTI_THREAD_BOT_H
#define TEST_MULTI_THREAD_BOT_H

#include <sc2api/sc2_api.h>
#include <functional>
#include <iostream>
#include <string>
#include <atomic>
#include "../../rolling_bot/simulator/simulator.h"
#include "list"

namespace sc2
{

class TestMultiThreadBot : public Agent
{
public:
    TestMultiThreadBot(int size,
                       const std::string &net_address,
                       int port_start,
                       const std::string &process_path,
                       const std::string &map_path) : m_simulators(size, net_address, port_start, process_path, map_path),
                                                      m_positions_in_each_sims(size)
    {
        m_simulators.StartAsync();
    }

    int RandomActionsAllSims(const std::chrono::milliseconds &ms)
    {
        //todo start function async
        m_end_flag.store(false);
        size_t sz = m_simulators.simulators.size();
        for (size_t i = 0; i < sz; i++)
        {
            m_futures.emplace_back(std::async(std::launch::async, &TestMultiThreadBot::RandomActionsASim, this, std::ref(m_simulators.simulators[i])));
        }

        if (ms <= std::chrono::milliseconds(0))
        {
            //! infinite time
            ;
        }
        else
        {
            // finite time
            std::this_thread::sleep_for(ms);
            m_end_flag.store(true);
            std::chrono::time_point<std::chrono::steady_clock> deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
            std::cout << "deadline + 5s" << std::endl;
            for (auto it = m_futures.begin(); it != m_futures.end();)
            {
                std::future_status status = (*it).wait_until(deadline);
                switch (status)
                {
                case std::future_status::ready:
                {
                    it = m_futures.erase(it);
                    // std::lock_guard<std::mutex> lk(m_cout_mutex);
                    //std::cout << "sim " << i << " is ready!" << std::endl;
                    // output the positions
                    {
                        // std::list<Point2D> ls = (*it).get();
                        // const int pos_count = 5;
                        
                        // std::cout << "the first " << pos_count << " positions: ";
                        // for (size_t i = 0; i < pos_count && i < ls.size(); i++)
                        // {
                        //     Point2D pt = ls.front();
                        //     std::cout << "( " << pt.x << ", " << pt.y << " )"
                        //               << "\t";
                        //     ls.pop_front();
                        // }
                        // std::cout << std::endl;
                    }
                    break;
                }
                case std::future_status::deferred:
                {
                    std::lock_guard<std::mutex> lk(m_cout_mutex);
                    std::cout << "sim is deferred!" << std::endl;
                    ++it;
                    break;
                }
                case std::future_status::timeout:
                {
                    std::lock_guard<std::mutex> lk(m_cout_mutex);
                    std::cout << "sim is timeout!" << std::endl;
                    // create a valid thread and swap the stuck thread with it
                    ++it;
                    break;
                }
                default:
                    break;
                }
            }
        }
        return m_futures.size();
    }

    std::list<Point2D> RandomActionsASim(Simulator &sim)
    {
        // get a unit
        for (size_t i = 0; i < 20; i++)
        {
            sim.Update();
        }
        std::list<Point2D> positions;
        if (sim.Observation()->GetUnits(Unit::Alliance::Self).empty())
        {
            std::cerr << "there is no self unit in this map!" << std::endl;
            return std::list<Point2D>();
        }
        const Unit *u = sim.Observation()->GetUnits(Unit::Alliance::Self).front();
        const GameInfo &game_info = sim.Observation()->GetGameInfo();
        Point2D map_max = game_info.playable_max;
        Point2D map_min = game_info.playable_min;
        while (!m_end_flag.load())
        {
            // choose a random pos to move to
            Point2D rand_pos = FindRandomLocation(map_min, map_max);
            sim.Actions()->UnitCommand(u, ABILITY_ID::MOVE, rand_pos);
            // update the game for a while
            for (size_t i = 0; i < m_update_loop_in_each_action; i++)
            {
                sim.Update();
            }
            positions.emplace_back(u->pos);
        }
        return positions;
    }

private:
    int m_update_loop_in_each_action = 24;
    std::atomic<bool> m_end_flag{false};
    Simulators m_simulators;
    std::list<std::future<std::list<Point2D>>> m_futures;
    std::mutex m_cout_mutex;
    // store the only one unit's movement
    std::vector<std::list<Point2D>> m_positions_in_each_sims;
};
} // namespace sc2
#endif // !TEST_MULTI_THREAD_BOT_H