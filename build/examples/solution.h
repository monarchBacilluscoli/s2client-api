#ifndef SOLUTION_H
#define SOLUTION_H

#include "sc2api/sc2_api.h"



namespace sc2 {
    // a series of actions for a unit
    struct command {
        Tag unit_tag;
        RawActions actions;
    };
    using commands = std::vector<command>;

    // many command make up a sulution
    class solution {
    public:
        solution() = default;
        ~solution() = default;
        const commands& get_commands() {
            return m_commands;
        }
        void set_commands(const commands& c) {
            m_commands = c;
        }
        void get_command_of_unit(Tag t) {
            
        }
    private:
        commands m_commands;
    };


}




#endif // !SOLUTION_H


