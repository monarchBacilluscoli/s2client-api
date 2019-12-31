#ifndef ROLLING_SOLUTION_H
#define ROLLING_SOLUTION_H

#include "solution.h"
#include "../simulator/command.h"
#include "../simulator/statistical_data.h"
#include <vector>

namespace sc2
{

struct SimUnitData{
    Unit final_state;
    UnitStatisticalData statistics;
};
struct SimGameData{
    GameResult result;
};
struct SimData{
    std::map<Tag, SimUnitData> units;
    SimGameData game;
};

template <class T = Command>
struct RollingSolution : public Solution<T>
{

    RollingSolution() : Solution<T>(){};
    RollingSolution(const RollingSolution<T> &rhs) = default;
    RollingSolution(const std::vector<T> &variable) : Solution<T>(variable){};
    RollingSolution(int variable_size, int objective_size) : Solution<T>(variable_size, objective_size){};
    ~RollingSolution() = default;

    std::vector<SimData> results; // multiple times of simulations
};

} // namespace sc2

#endif