#ifndef POTENTIAL_FIELD_BOT
#define POTENTIAL_FIELD_BOT

#include<iostream>
#include<sc2api/sc2_api.h>

namespace sc2 {
    class potential_field_bot:public Agent {
    public:
        potential_field_bot(int step_size = 1):m_step_size(step_size) {};
        virtual ~potential_field_bot() = default;

        Vector2D force_to_unit(const Units& sources, const Unit* u);

        virtual void OnGameStart();
        virtual void OnStep();

        const Unit* select_nearest_unit_from_point(const Point2D& p, const Units& us);
        //todo search units can be attacked
        Units search_units_can_be_attacked_by_unit(const Unit* u, Unit::Alliance a = Unit::Alliance::Enemy);
        //

    protected:
        Vector2D force_enemy_to_unit(const Unit* source, const Unit* target);
        Vector2D force_ally_to_unit(const Unit* source, const Unit* target);
        Vector2D force_wall_to_unit(const Unit* target);

        virtual float calculate_enemy_attraction_value(const Unit* enemy, const Unit* unit) const { return 0.f; };
        virtual float calculate_enemy_repulsion_value(const Unit* enemy, const Unit* unit) const { return 0.f; };
        virtual float calculate_ally_attraction_value(const Unit* ally, const Unit* unit) const { return 0.f; };
        virtual float calculate_ally_repulsion_value(const Unit* ally, const Unit* unit) const { return 0.f; };

        float calculate_enemy_zero_field_dis(const Unit* source, const Unit* target);
        float calculate_ally_zero_field_dis(const Unit* source, const Unit* target);

        float move_dis_per_time_slice(const Unit* u) const;

        Units serach_enemies_can_be_attacked_by_unit(const Unit* u);

        void display_facing_direction(const Units& us, DebugInterface* debug);
        void display_force_direction(const Unit* u, const Vector2D& force,DebugInterface* debug);
        void display_field_boundary(const Unit* selected_u, const Units& source_us, DebugInterface* debug);
        void display_fire_range(const Units& us, DebugInterface* debug);
        void display_playable_area(const GameInfo& gi, DebugInterface* debug);

        const float m_zero_potential_energy_ratio = 0.8f;
        const float m_zero_ally_field_ratio = 0.6f;
        const float m_wall_force_factor = 10; // must be big enough

        Units m_alive_self_units;
        Units m_alive_enemy_units;
        Units m_all_alive_units;
        UnitTypes m_unit_types;
        GameInfo m_game_info;

        /*
         * some additional data members used in derived class
        */
        Units m_initial_units;
        Units m_initial_self_units;
        Units m_initial_enemy_units;

        /*
        * game process configuration
        */
        int m_step_size;
    };
}

#endif // !POTENTIAL_FIELD_BOT
