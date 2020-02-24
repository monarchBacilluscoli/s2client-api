// This file is only used for debugging, and will not exist in program in future

#ifdef DEBUG

#ifndef DEBUG_USE_H
#define DEBUG_USE_H

#include "rolling_bot/simulator/simulator_pool.h"

namespace sc2
{
void check_unit_tag_same(const std::vector<Solution<Command>> &pop, const Solution<Command> *sol = nullptr) // check if the order of units is the same with a solution in a population
{
    if (!sol)
    {
        sol = &(pop.front());
    }
    for (auto &&i : pop)
    {
        bool is_equal = std::equal(i.variable.begin(), i.variable.end(), sol->variable.begin(), sol->variable.end(), [](const Command &c1, const Command &c2) -> auto {
            return c1.unit_tag == c2.unit_tag;
        });
        if (!is_equal)
        {
            throw(std::string("unit order in variables must be the same@") + __FUNCTION__);
        }
    }
}

} // namespace sc2

#endif // DEBUG_USE_H
#endif // DEBUG