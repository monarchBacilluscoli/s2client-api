// This file is only used for debugging, and will not exist in program in future

#ifdef DEBUG

#ifndef DEBUG_USE_H
#define DEBUG_USE_H

#include <iostream>

#include "rolling_bot/simulator/simulator_pool.h"

namespace sc2
{
void CheckUnitTagSame(const std::vector<Solution<Command>> &pop, const Solution<Command> *sol = nullptr) // check if the order of units is the same with a solution in a population
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

void OutputAllStatistics(const std::vector<RollingSolution<Command>> &pop, std::ostream &os)
{
    os << std::endl;
    int sz = pop.size();
    std::vector<float> ave_action_nums(pop.front().results.front().units.size(), 0.f);
    std::vector<float> ave_attack_nums(ave_action_nums.size(), 0.f);
    int team_attack_nums;
    int total_sims = 0;

    for (size_t i = 0; i < sz; i++)
    {
        int sz_sims = pop[i].results.size();

        for (size_t j = 0; j < sz_sims; j++)
        {
            ++total_sims;
            // os << "result " << j << ": " << '\t';
            const auto &units = pop[i].results[j].units;
            int u_count = 0;
            for (const auto &u : units)
            {
                ave_action_nums[u_count] += u.second.statistics.action_number;
                ave_attack_nums[u_count] += u.second.statistics.attack_number;
                ++u_count;
                // os << u.second.statistics.action_number << '\t' << u.second.statistics.attack_number << '\t' << u.second.statistics.events.actions.size() << std::endl;
            }
        }
    }
    //todo output average.
    os << "aveage num: " << std::endl;
    int u_sz = ave_action_nums.size();
    for (int i = 0; i < u_sz; i++)
    {
        os << ave_action_nums[i] / total_sims << '\t' << ave_attack_nums[i] / total_sims << std::endl;
    }
    os << "team attack number: " << '\t' << std::accumulate(ave_attack_nums.begin(), ave_attack_nums.end(), 0) / total_sims;
    os << std::endl;
}

void OutputEvents(const RollingSolution<> &s, std::ostream &os)
{
    ;
}

} // namespace sc2

#endif // DEBUG_USE_H
#endif // DEBUG