/*! \file command.h
    \brief an action series for a unit to take, provided to algorithms to optimize

    Here should be a discription of this class's functions
*/

#ifndef COMMAND_H
#define COMMAND_H

#include<sc2api/sc2_gametypes.h>
#include<sc2api/sc2_action.h>

namespace sc2 {
    struct Command {
        Tag unit_tag = 0;
        RawActions actions = RawActions();

        bool operator==(const Command& rhs)const {
            return unit_tag == rhs.unit_tag && actions == rhs.actions;
        };
        bool operator!=(const Command& rhs)const {
            return !(*this == rhs);
        };
    };
}


#endif // !COMMAND


