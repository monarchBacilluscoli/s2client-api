#ifndef ROLLING_SOLUTION_H
#define ROLLING_SOLUTION_H

#include "solution.h"
#include "../simulator/command.h"
#include "../simulator/statistical_data.h"
#include "../../../include/sc2api/sc2_unit.h"

namespace sc2
{

struct SimUnitData
{
    Unit final_state;
    UnitStatisticalData statistics;
};
struct SimGameData
{
    GameResult result;
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

    RollingSolution() : Solution<T>(){};
    RollingSolution(const RollingSolution<T> &rhs) = default;
    RollingSolution(const std::vector<T> &variable) : Solution<T>(variable){};
    RollingSolution(int variable_size, int objective_size) : Solution<T>(variable_size, objective_size){};
    virtual ~RollingSolution() = default;

    void ClearSimData();
    void CalculateAver();

    static bool AttackCountLess(const RollingSolution<T> &first, const RollingSolution<T> &second);
    static bool HealthChangeLess(const RollingSolution<T> &first, const RollingSolution<T> &second);
    //todo DifferenceLess
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

} // namespace sc2

#endif // ROOLING_SOLUTION_H