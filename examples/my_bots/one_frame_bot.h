#ifndef ONE_FRAME_BOT_H
#define ONE_FRAME_BOT_H

#include<iostream>
#include<sc2api/sc2_api.h>
#include<map>
#include"solution.h"
#include<functional>

//#define DEBUG
//#define MULTI_THREAD

using namespace std::placeholders;

namespace sc2 {
    class one_frame_bot : public Agent {

        using population = std::vector<solution>;
        using evaluator = std::function<float(const solution& s)>;

    public:
        one_frame_bot() {
            m_population.reserve(m_population_size + m_offspring_size);
            m_damage_objective.reserve(m_population_size + m_offspring_size);
            m_threat_objective.reserve(m_population_size + m_offspring_size);

            //! MUST register evaluators here FIRST!
            m_evaluators.resize(2);
            //? why I should use &one_frame_bot rather than this-> ???
            m_evaluators[0] = std::bind(&one_frame_bot::evaluate_single_solution_damage_next_frame, this, _1);
            m_evaluators[1] = std::bind(&one_frame_bot::evaluate_single_solution_threat_next_frame, this, _1);
        }
        ~one_frame_bot() = default;

        virtual void OnGameStart() final;
        virtual void OnStep() final;
        //todo control units by rules
        virtual void OnUnitIdle(const Unit * u) final;
        virtual void OnGameEnd() final;

    private:
        solution run();

        // generate a random solution
        solution generate_random_solution();

        // generate random solutions
        void generate_random_solutions(population & pop, size_t size);
        // population generate population
        void generate_offspring(const population & parents, population & offspring, int spring_size);

        // two parents generate two children
        std::vector<solution> produce(const solution & a, const solution & b);
        // two parents generate two children (unmutated)
        //? 
        std::vector<solution> cross_over(const solution & a, const solution & b);
        // mutate
        void mutate(solution & s);

        //todo evaluate
        void evaluate_solutions(population & p);

        void sort_solutions(population& p, std::function<bool(const solution& a, const solution& b)> compare);

            //solution select_one_solution(const population& p);
            //todo deploy
            void deploy_solution(const solution & s);

        // evaluate damage to opponent of single solution
        float evaluate_single_solution_damage_next_frame(const solution & s);
        // evaluate the hurt of us of a single solution
        float evaluate_single_solution_hurt_next_frame(const solution & s);
        //
        float evaluate_single_solution_threat_next_frame(const solution & s);
        // evaluate potential advantage caused by movement next frame
        float evaluate_movement_advantage_next_frame(const solution & s);


        // some utility functions don't belong to the evolutionay algorithm
        // search neighboring units (using the positions in a solution)
        Units search_units_within_radius_in_solution(const Point2D & p, float r, const solution & s);
        // search my units can be attacked
        Units search_units_can_be_attacked_by_weapon_in_solution(const Point2D & p, Weapon w, const solution & s); //! one frame edtion
        Units search_units_can_be_attacked_by_unit_in_solution(const Unit * u, const solution & s); //! one frame edition
        Units serach_units_can_be_attacked_by_unit(const Unit * u, Unit::Alliance a);

        // search neighboring units (using actual positions on map)
        Units search_units_within_radius(const Point2D & p, float r, Unit::Alliance a);
        // generate a random point in a circle
        Point2D generate_random_point_within_radius(float r);
        // search the nearest unit on map
        const Unit * search_nearest_unit_from_point(const Point2D & p, Unit::Alliance a, Filter f = {});

        //
        const Unit * select_lowest_hp_enemy(const Units & us);
        //
        const Unit * select_nearest_unit_from_point_(const Point2D & p, const Units & us);

        // consider is_flying
        bool is_weapon_match_unit(const Weapon & w, const Unit * u);

        // get all the weapons of attack which can be used to attack the target
        std::vector<Weapon> get_matched_weapons_without_considering_distance(const Unit * attack, const Unit * target);
        //
        Weapon get_longest_range_weapon_of_unit_type(const UnitTypeData & ut);
        Weapon get_longest_range_weapon_of_weapons(const std::vector<Weapon> ws);
        //
        bool is_attackable(const Unit * attacking_u, const Unit * target_u);

        //
        float damage_weapon_to_unit(const Weapon & w, const Unit * u);
        //
        float damage_unit_to_unit_without_considering_distance(const Unit * attacking_u, const Unit * target_u);
        //
        float damage_unit_to_unit(const Unit * attacking_u, const Unit * target_u);
        //
        float basic_movement_one_frame(const UnitTypeData & ut);
        //
        Point2D calculate_pos_next_frame(const Unit * u, const Point2D & p);
        // see if the weapon w can be used to attack unit u

        // real distance between two units (without two radiuses)
        float real_distance_between_two_units(const Unit * u1, const Unit * u2);


        float threat_from_unit_to_unit(const Unit * source_u, const Unit * target_u);
        float threat_from_unit_to_unit_new_pos(const Unit * source_u, const Unit * target_u, const Point2D & pos);

        float threat_from_units_to_unit(const Units & source_us, const Unit * target_u);
        float threat_from_units_to_unit_new_pos(const Units & source_us, const Unit * target_u, const Point2D & pos);
        //
        float calculate_zero_potential_field_distance(const Unit * source_u, const Unit * target_u);

        const Unit * get_execution_unit(const command & c);


        // for debug
        void display_fire_range(DebugInterface * debug, const Units & all_units);
        void display_units_collision_range(DebugInterface * debug, const Units & us);
        void display_units_pos(DebugInterface * debug, const Units & us);
        void display_map_grid(const GameInfo & info);
        void display_unis_actions();
        void display_units_move_action(DebugInterface * debug, const Units & us);
        void display_units_attack_action(DebugInterface * debug, const Units & us);
        void display_solution_line(DebugInterface * debug, const solution s);
        void display_movement(DebugInterface * debug, const Units & us);



    private:
        const double PI = atan(1.) * 4.;
        // Some data from the proto is presented in old 'normal' speed, now I should reset them to the faster speed. 
        const float m_frames_per_second = 16;

        // game data should be updated
        Units m_alive_self_units;
        Units m_alive_enemy_units;
        Units m_all_alive_units;
        bool m_is_save = false; // for forward model
        // constant game data
        GameInfo m_game_info;
        UnitTypes m_unit_types;

        UnitTypeData m_marine; // for convenient

        // algorithm configuration
        const int m_frames_per_deploy = 12;
        const int command_length = 1;
        const float zero_potential_energy_ratio = 0.8f;
        const double m_attack_prob = 1;
        const double m_killing_bonus_factor = 3;

        const int m_objective_number = 2; //! this must be properly set before it running
        const int m_population_size = 20;
        const int m_offspring_size = 20;
        const float m_muatation_rate = 0.1f;
        const float m_crossover_rate = 1;
        const double m_theta_mutate_step = 2 * PI / 10.;
        const int m_produce_times = 20;



        // algorithm content
        population m_population;
        std::vector<float> m_damage_objective;
        std::vector<float> m_threat_objective;
        solution m_selected_solution; //todo: useless
        std::vector<evaluator> m_evaluators;
    };

}


#endif // !ONE_FRAME_BOT_H



