#ifndef ADVANCED_POTENTIAL_FIELD_BOT_H
#define ADVANCED_POTENTIAL_FIELD_BOT_H

#include "potential_field.h"

namespace sc2 {
    class advanced_potential_field_bot :public potential_field_bot {
    public:
        advanced_potential_field_bot(int step_size):potential_field_bot(step_size) {}
        ~advanced_potential_field_bot() = default;

        Vector2D force_enemy_to_unit(const Unit* source, const Unit* target);
        Vector2D force_ally_to_unit(const Unit* source, const Unit* target);
        Vector2D force_wall_to_unit(const Unit* target);

    private:

        float calculate_enemy_attraction_value(const Unit* enemy, const Unit* unit) const;
        float calculate_enemy_repulsion_value(const Unit* enemy, const Unit* unit) const;
        float calculate_friendly_attraction_value(const Unit* ally, const Unit* unit) const;

        /*
        * the field configuration
        */
        // parameters of friendly unit attraction
        float m_fa_cost_facotr = 1.f; //? maybe I can convert it into damage value...only if I can find the relationship between cost and HP+DMG+SHD+AMR....
        float m_fa_range_factor = 1.f;
        //? maybe a fixed value factor?
        // field parameters of enemy attaction
        float m_ea_range_factor = 1.f;
        float m_ea_max_factor = 1.f;
        float m_ea_distance_factor = 1.f;
        //? maybe a fixed value factor?
        // field parameters of enemy repulsive
        float m_er_max_factor = 1.f;
        float m_er_range_factor = 1.f;
        // target select parameter
        float m_wasted_damage_threshold;


    };
}

#endif // !ADVANCED_POTENTIAL_FIELD_BOT_H
