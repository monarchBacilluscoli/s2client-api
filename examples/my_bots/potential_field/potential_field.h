#ifndef POTENTIAL_FIELD_BOT
#define POTENTIAL_FIELD_BOT

#include<iostream>
#include<sc2api/sc2_api.h>

namespace sc2 {
    class potential_field_bot:public Agent {
    public:
        potential_field_bot() = default;
        ~potential_field_bot() = default;

        Vector2D force_enemy_to_unit(const Unit* source, const Unit* target);
        Vector2D force_ally_to_unit(const Unit* source, const Unit* target);

        Vector2D force_to_unit(const Units& sources, const Unit* u);
        float calculate_enemy_zero_field_dis(const Unit* source, const Unit* target);
        float calculate_ally_zero_field_dis(const Unit* source, const Unit* target);
        Units serach_enemies_can_be_attacked_by_unit(const Unit* u);


        virtual void OnGameStart() final;
        virtual void OnStep() final;

        const Unit* select_nearest_unit_from_point(const Point2D& p, const Units& us);


    private:

        void display_facing_direction(const Units& us, DebugInterface* debug);
        void display_force_direction(const Unit* u, const Vector2D& force,DebugInterface* debug);
        void display_field_boundary(const Unit* selected_u, const Units& source_us, DebugInterface* debug);
        void display_fire_range(const Units& us, DebugInterface* debug);

        const float zero_potential_energy_ratio = 0.8f;
        const float zero_ally_field_ratio = 0.6f;

        Units m_alive_self_units;
        Units m_alive_enemy_units;
        Units m_all_alive_units;
        UnitTypes m_unit_types;

    };
}

#endif // !POTENTIAL_FIELD_BOT
