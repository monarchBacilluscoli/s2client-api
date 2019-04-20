#ifndef SOLUTION_H
#define SOLUTION_H

#include "sc2api/sc2_api.h"
#include <numeric>

namespace sc2 {
    // a series of actions for a unit
    struct command {
        Tag unit_tag;
        RawActions actions;

        bool operator==(const command& rhs)const;
        bool operator!=(const command& rhs)const;
    };

    // many command make up a sulution
    struct solution {
        solution() = default;
        solution(int command_size, int objective_size) {
            commands.resize(command_size);
            objectives.resize(objective_size);
        }
        solution(int command_size) {
            commands.resize(command_size);
        }
        ~solution() = default;

        std::vector<float> objectives;
        std::vector<command> commands;
        int rank = 0;

        bool operator==(const solution& rhs) const;
        bool operator!=(const solution& rhs) const;
    };

    bool multi_greater(const solution& a, const solution& b);

    // Simply add up all the objectives without considering weights
    bool simple_sum_greater(const solution& a, const solution& b);
    //! just for test.
    //bool no_compare(const solution& a, const solution& b);
}

#endif // !SOLUTION_H


