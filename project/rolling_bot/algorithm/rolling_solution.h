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
    ~RollingSolution() = default;

    void ClearSimData();
    void CalculateAver();

    static bool AttackCountLess(const RollingSolution<T> &first, const RollingSolution<T> &second);
    static bool HealthChangeLess(const RollingSolution<T> &first, const RollingSolution<T> &second);
    //todo DifferenceLess
};

} // namespace sc2

#endif // ROOLING_SOLUTION_H