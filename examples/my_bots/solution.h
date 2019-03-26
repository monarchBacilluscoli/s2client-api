#ifndef SOLUTION_H
#define SOLUTION_H

#include "sc2api/sc2_api.h"

namespace sc2 {
    // a series of actions for a unit
    struct Command {
        Tag unit_tag;
        RawActions actions;
    };

    // many command make up a sulution
    struct Solution {
        Solution(int command_size) {
            commands.resize(command_size);
        }
        ~Solution() = default;

        std::vector<float> objectives;
        std::vector<Command> commands;
    };
}




#endif // !SOLUTION_H


