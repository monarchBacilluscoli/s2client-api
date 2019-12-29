#ifndef ROLLING_SOLUTION_H
#define ROLLING_SOLUTION_H

#include "solution.h"
#include "../simulator/command.h"
#include <vector>

namespace sc2
{
template <class T = Command>
struct RollingSolution : public Solution<T>
{
private:
    std::vector<int> iron_rank_per_unit;
    std::vector<int> normal_rank_per_unit;
    std::vector<int> run_rank_per_unit;

public:
    RollingSolution() : Solution<T>(){};
    RollingSolution(const RollingSolution<T> &rhs) = default;
    RollingSolution(const std::vector<T> &variable) : Solution<T>(variable){};
    RollingSolution(int variable_size, int objective_size): Solution<T>(variable_size, objective_size){};
    ~RollingSolution() = default;
};

} // namespace sc2

#endif