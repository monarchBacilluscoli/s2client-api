#ifndef RULE_BASED_BOT_H
#define RULE_BASED_BOT_H

#include<iostream>
#include<sc2api/sc2_api.h>

namespace sc2 {
    // contains some basic methods
    class rule_based_bot :public Agent {
    public:
        rule_based_bot() = default;
        ~rule_based_bot() = default;

        //todo since it has been moved to sc2_utility, it would better be deleted here
        const Unit* select_nearest_unit_from_point(const Point2D& p, const Units& us);

    protected:
        Units m_alive_self_units;
        Units m_alive_enemy_units;
        Units m_all_alive_units;
        UnitTypes m_unit_types;
        GameInfo m_game_info;
    };
}

#endif // !RULE_BASED_BOT_H
