#ifndef ONE_FRAME_BOT_H
#define ONE_FRAME_BOT_H

#include<iostream>
#include<sc2api/sc2_api.h>

namespace sc2 {
    class one_frame_bot: public Agent {
        using solution = std::map<Tag, RawActions>;
        using population = std::vector<solution>;

    public:
        one_frame_bot() = default;
        ~one_frame_bot() = default;

        virtual void OnGameStart() final;
        virtual void OnStep() final;
        // control units by rules
        virtual void OnUnitIdle(const Unit* u) final;

    private:
        //todo generate a random solutions
        solution generate_random_solution();
        //todo two parents generate two children
        std::vector<solution> cross_over(const solution& a, const solution& b);
        //todo mutate
        solution mutate(const solution& s);

        // evaluate
        void evaluate_all_solutions(const population& p, std::vector<float>& d, std::vector<float>& h);
        //
        void sort_solutions(population& p, std::vector<float>& d, std::vector<float>& h);
        //
        solution select_one_solution(const population& p, std::vector<float>& d, std::vector<float>& h);
        //todo todo
        void deploy_solution(const solution& s);

        //todo evaluate damage to opponent of single solution
        float evaluate_single_solution_damage(const solution& s);
        //todo evalute the hurt of us of a single solution
        float evaluate_single_solution_hurt(const solution& s);
        

        // some utility functions don't belong to the evolutionay algorithm
        //todo search neighboring units (using the positions in a solution)
        Units search_units_within_radius_in_solution(const Point2D& p, float r, const solution& s);

        // search neighboring units (using actual positions on map)
        Units search_units_within_radius(const Point2D& p, float r, Unit::Alliance a);
        // generate a random point in a circle
        Point2D generate_random_point_within_radius(float r);
        // search the nearest unit on map
        const Unit* search_nearest_unit_from_point(const Point2D& p, Unit::Alliance a, Filter f = {});

        //
        const Unit* select_lowest_hp_enemy(const Units& us);
        //
        const Unit* select_nearest_unit_from_point_(const Point2D& p, const Units& us);

        //
        float damage_weapon_to_unit(const Weapon& w, const Unit* u);

        // for debug
        void display_fire_range(const Units& all_units);
        void display_units_collision_range(const Units& us);
        void display_units_pos(const Units& us);
        void display_map_grid(const GameInfo& info);
        void display_unis_actions();
        void display_units_move_action(const Units& us);
        void display_units_attack_action(const Units& us);
        void display_solution_line(const solution s);



        

    private:
        const float PI = 3.1415926;

        // game data should be updated
        Units m_alive_self_units;
        Units m_alive_enemy_units;
        Units m_all_alive_units;
        bool m_is_save = false; // for forward model
        // static game data
        GameInfo m_game_info;
        UnitTypes m_unit_types;
        UnitTypeData m_marine; // for convenient

        // algorithm configuration
        int m_frames_per_deploy = 12;
        int m_population_size = 50;
        float m_muatation_rate = 0.05;
        float m_crossover_rate = 1;
        int produce_times = 50;
        int command_length = 1;

        // algorithm content
        population m_population;
        std::vector<float> m_hurt_objective;
        std::vector<float> m_damage_objective;
        solution m_selected_solution; //todo: useless
    };

}


#endif // !ONE_FRAME_BOT_H



