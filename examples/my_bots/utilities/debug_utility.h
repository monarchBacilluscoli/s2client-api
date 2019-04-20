#ifndef DEBUG_UTILITY_H
#define DEBUG_UTILITY_H

#include"sc2api/sc2_api.h"
#include"../solution.h"

namespace sc2 {
    void display_fire_range(DebugInterface* debug, const Units& us, const UnitTypes& utd);
    void display_units_collision_range(DebugInterface* debug, const Units& us);
    void display_units_pos(DebugInterface* debug, const Units& us, bool need_select = false);
    void display_map_grid(const GameInfo& info);
    void display_unis_actions();
    void display_units_move_action(DebugInterface* debug, const Units& us);
    void display_units_attack_action(DebugInterface* debug, const Units& us, bool need_select = false);
    void display_solution_line(DebugInterface* debug, const solution s);
    void display_movement(DebugInterface* debug, const Units& us);

}

#endif // !DEBUG_UTILITY_H
