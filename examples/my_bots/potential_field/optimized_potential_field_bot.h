#ifndef OPTIMIZED_POTENTIAL_FIELD_BOT
#define OPTIMIZED_POTENTIAL_FIELD_BOT

#include"potential_field.h"

namespace sc2 {
    class optimized_potential_field_bot: public potential_field_bot {
    public:
        //todo the initializer must be modified
        //todo it should contain all parameters it needs
        //todo
        optimized_potential_field_bot() = default;
        ~optimized_potential_field_bot() = default;

        //? should I use the dependency injection?
        //! NO! Here shouldn't be an evaluation.
        /*void evaluate();*/
        void OnGameStart() final;
        void OnStep() final;

        //todo calculate the weighted health lose of my units
        int calculate_weighted_health_lose(); //? it must contain shield loss
        //todo calculate the weighted health lose of enemy units
        int calculate_weighted_damage_done(); //? contain shield loss


    private:
        //todo calculate the next position
        Vector2D get_next_position(const Unit* u);
        //todo get the target unit
        const Unit* get_target_enemy(const Unit* u);


        /*
         * the field configuration
        */
        // parameters of friendly unit attraction
        float m_fa_no_field_dis;
        float m_fa_field_increasing_dis;
        float m_fa_slope;
        float m_fa_base_force;
        // field parameters of enemy attaction
        float m_ea_range_factor;
        float m_ea_slope;
        float m_ea_fragile_factor;
        float m_ea_damage_factor;
        float m_ea_base_force;
        // field parameters of enemy repulsive
        float m_er_range_factor;
        float m_er_slope;
        float m_er_fragile_facotr;
        float m_er_damage_factor;
        float m_er_base_force;
        // target select parameter
        float m_wasted_damage_threshold;

        /*
         * the data used in game
         */

    };
}

#endif // !OPTIMIZED_POTENTIAL_FIELD_BOT
