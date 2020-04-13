#ifndef ROLLING_SOLUTION_H
#define ROLLING_SOLUTION_H

#include <iostream>
#include "solution.h"
#include "../simulator/command.h"
#include "../simulator/statistical_data.h"
#include "../../../include/sc2api/sc2_unit.h"
#include <sc2lib/sc2_utils.h>

namespace sc2
{

struct SimUnitData
{
    Unit final_state;               // the final state of the unit
    UnitStatisticalData statistics; // the statictical data during simulation
};
struct SimGameData
{
    GameResult result = GameResult::Undecided;
    u_int32_t end_loop = std::numeric_limits<u_int32_t>::max();
};
struct SimData
{
    std::map<Tag, SimUnitData> units;
    SimGameData game;
};
struct AverageSimData
{
    std::map<Tag, UnitStatisticalData> units_statistics;
    float win_rate; //? how to define it? how to handle tie?

    static std::map<Tag, UnitStatisticalData> CalculateAverUnitStatistics(const std::vector<SimData> &results);
    static float CalculateWinRate(const std::vector<SimData> &results);
};

template <class T = Command>
struct RollingSolution : public Solution<T>
{
    std::vector<SimData> results; // results of multiple times of simulations
    AverageSimData aver_result;   // must be calculated by calling CalculateAver() before being used
    bool is_priori = false;

    RollingSolution() : Solution<T>(){};
    RollingSolution(const RollingSolution<T> &rhs) = default;
    RollingSolution(const std::vector<T> &variable) : Solution<T>(variable){};
    RollingSolution(int variable_size, int objective_size) : Solution<T>(variable_size, objective_size){};
    virtual ~RollingSolution() = default;

    void ClearSimData();
    void CalculateAver();

    static bool AttackCountLess(const RollingSolution<T> &first, const RollingSolution<T> &second);
    static bool HealthChangeLess(const RollingSolution<T> &first, const RollingSolution<T> &second);
    static bool RollingLess(const RollingSolution<T> &l, const RollingSolution<T> &r);
    //todo DifferenceLess

    // utilities
    Point2D GetUnitPossiablePosition(Tag tag, u_int32_t game_loop);
};

template <class T>
void RollingSolution<T>::ClearSimData()
{
    aver_result.units_statistics.clear();
    aver_result.win_rate = 0.f;
    results.clear();
}

template <class T>
void RollingSolution<T>::CalculateAver()
{
#ifdef DEBUG
    int sim_sz = results.size();
    if (sim_sz > 1)
    {
        for (size_t i = 0; i + 1 < sim_sz; ++i) // to check if all the results have the same size of units statistical data
        {
            if (results[i].units.size() != results[i + 1].units.size())
            {
                throw(std::string("there is something wrong in recording game result@") + __FUNCTION__);
            }
        }
    }
#endif // DEBUG
    aver_result.units_statistics = AverageSimData::CalculateAverUnitStatistics(results);
    aver_result.win_rate = AverageSimData::CalculateWinRate(results);
}

template <class T>
Point2D RollingSolution<T>::GetUnitPossiablePosition(Tag tag, u_int32_t game_loop)
{
    Point2D pos(0, 0);
    // only take action into consideration
    int results_sz = results.size();
    for (size_t i = 0; i < results_sz; i++) // multiple simulations, multiple results
    {
        const auto &action_list = results[i].units.at(tag).statistics.events.actions;
        std::list<Events::Action>::const_iterator it = std::find_if_not(action_list.begin(), action_list.end(), [game_loop](const Events::Action &a) -> bool { return a.gameLoop() < game_loop; });
        if (it != action_list.end() && std::distance(action_list.end(), it) > 1) // not the last one or not all the action's gameloop is less than it
        {
            if ((it == action_list.begin()) || it->gameLoop() == game_loop)
            {
                pos += it->position();
            }
            else
            {
                const Events::Action &a2 = *it;
                const Events::Action &a1 = *(--it);
                pos += CalcPointOnLineByRatio(a1.position(), a2.position(), (float)(game_loop - a1.gameLoop()) / (float)(a2.gameLoop() - a1.gameLoop()));
            }
        }
        else
        {
            pos += action_list.back().position();
        }
    }
    return pos / results_sz;
}

template <class T>
bool RollingSolution<T>::RollingLess(const RollingSolution<T> &l, const RollingSolution<T> &r)
{
    if (l.rank != r.rank)
    {
        return l.rank < r.rank;
    }
    else
    {
        // Compare objectives
        if (l.objectives.size() != r.objectives.size())
        {
            throw("The obj sizes of the two solution are not the same @ Solution::" + std::string(__FUNCTION__));
        }
        else if (l.results.front().game.result == GameResult::Win || r.results.front().game.result == GameResult::Win)
        {
            // std::cout << "sort loop" << std::endl;
            auto loop_add = [](u_int32_t sum, const SimData &d) -> u_int32_t { return sum + d.game.end_loop; };
            // return std::accumulate(l.results.begin(), l.results.end(), 0, loop_add) < std::accumulate(r.results.begin(), r.results.end(), 0, loop_add);
            return l.results.front().game.end_loop < r.results.front().game.end_loop;
        }
        else
        {
            return false;
        }
    }
}

} // namespace sc2

#endif // ROOLING_SOLUTION_H