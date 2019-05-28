#ifndef SOLUTION_H
#define SOLUTION_H

#include "command.h"
#include <numeric>

namespace sc2 {
    // many commands make up a sulution
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
        std::vector<Command> commands;
        int rank = 0;

        bool operator==(const solution& rhs) const;
        bool operator!=(const solution& rhs) const;
    };

    static bool multi_greater(const solution& a, const solution& b) {
        bool equal = true;
        // If any of the b's dimensions is bigger than a, return false
        for (size_t i = 0; i < a.objectives.size(); i++) {
            if (a.objectives[i] < b.objectives[i]) {
                return false;
            }
            else if (equal && fabsf(a.objectives[i] - b.objectives[i]) > 0.000001) {
                equal = false;
            }
        }
        // If all the demensions are equal, return false
        if (equal) {
            return false;
        }
        // else return true
        return true;
    }

    // Simply add up all the objectives without considering weights
    static bool simple_sum_greater(const solution& a, const solution& b) {
        //! be careful to use std::accumulate and pay attention to the last parameter of this function, the return value's type depends on it.
        return std::accumulate(a.objectives.begin(), a.objectives.end(), 0.f) > std::accumulate(b.objectives.begin(), b.objectives.end(), 0.f);
    }
}



#endif // !SOLUTION_H


