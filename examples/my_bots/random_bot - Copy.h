#ifndef RANDOM_BOT_H
#define RANDOM_BOT_H

#include "sc2api/sc2_api.h"
#include <iostream>
#include <vector>

namespace sc2 {

    //// ���ݹ۲�״̬���ɽ�
    //class basic_solution {
    //public:
    //    basic_solution() = default;
    //    basic_solution(const std::vector<std::pair<Point2D, Unit*>>& c);
    //    void set_command(const std::vector<std::pair<Point2D, Unit*>>& c);
    //    void set_object(float f);
    //    std::pair<Point2D, Unit*> operator[](int i);
    //    void resize(int n);
    //    float fitness();
    //private:
    //    std::vector<std::pair<Point2D, Unit*>> m_command;
    //    float m_fitness = FLT_MIN;
    //};

    //// ���ݹ۲�״̬���ɽⲢ���ƽ������
    //class random_search {
    //public:
    //    random_search();
    //    // ����ÿ��n֡��ú�������һ���۲죬���Ӹú����õ�һ����
    //    void get_command(const ObservationInterface* ob);

    //    ~random_search();

    //private:
    //    // ���ݹ۲�ֵ���������
    //    std::vector<basic_solution> generate_random_solutions(const ObservationInterface* ob);
    //    // ��������ֵѡ�������
    //    void select_solution(std::vector<basic_solution> ss);

    //    basic_solution generate_random_solution(const ObservationInterface* ob);
    //    std::pair<float, float> generate_random_position_in_circle(float radius);
    //    int population_size = 10;
    //    int frames_per_deploy = 24;
    //};

    // ������Ϸ����
    class random_bot : public Agent {
    public:
        using command = std::tuple<const Unit*, Point2D, const Unit*>;
        using basic_solution = std::vector<command>;
        using population = std::vector<std::pair<basic_solution, float>>;

        //virtual void initialize(int deploy_solution);
        virtual void OnGameStart() final;
        virtual void OnStep() final; // �ؼ�����������Ϸ���У������㷨����
        virtual void OnUnitIdle(const Unit* unit) final;

    private:
        // ����һ�������
        void generate_random_solution();
        // ����һ�����ƶ����������λ�Ľ�
        void generate_attack_nearest_without_move_solution();
        // ����: ���ѡ���֮�е�һ����λָ��λ�ý��н���
        std::vector<basic_solution> cross_over(const basic_solution& a, const basic_solution& b);
        // ����: ����λ��
        basic_solution mutate(const basic_solution & s, float random_ratio_of_command, float random_range);
        
        // ������ǰ���н�
        void evaluate_all_solutions();
        // ��������ֵѡ������ֵ��ߵĽ�
        basic_solution select_solution();
        // ���ѡ��ĳ����
        basic_solution select_random_solution();
        // �����
        void deploy_solution(const basic_solution& s);

        // �����ҷ�����˺�
        float evaluate_single_solution_attack(const basic_solution& s);
        // �����ҷ��ܵ��˺�
        float evaluate_single_solution_hurt(const basic_solution& s);

        // ����Բ�ڲ������
        Point2D generate_random_point_in_circle(float r);
        // �ڵ�ͼ������ĳ��ĳ��Χ�ڵ���
        Units search_units_within_radius(const Point2D& p, Unit::Alliance a, float r);
        // �ڵ�ͼ����������ĳ��������ض���λ
        const Unit* search_nearest_unit_from_point(const Point2D& p, Unit::Alliance a, Filter f = {});

        // ѡ��hp���ٵĵ���
        const Unit* select_lowest_hp_enemy(const Units& us);
        // �ڸ�����λȺ���������������ĵ���
        const Unit* select_nearest_unit(const Point2D& p,const Units& us);
        
        // ������������ĳ���͵�λ���˺�
        float damage_weapon_to_unit(const Weapon& w, const Unit* u);

        /* Debug���躯�� */
        // ��ʾ������Χ
        void display_units_weapon_range(Units all_units);
        // ��ʾ��ײ��Χ
        void display_units_collision_range(Units us);
        // ��ʾ��λ����
        void display_units_pos(Units us);
        // ��ʾ��ͼ��������
        void display_map_grid(const GameInfo& info);
        // ��ʾ��λ����
        void display_units_actions();
        // ��ʾ��λ�ƶ�
        void display_units_move_action(Units us);
        // ��ʾ��λ����
        void display_units_attack_aciton(Units us);
        // ��ʾ���·�ߺ�Ŀ��
        void display_solution_line(const basic_solution& bs);

        // ����
        const float PI = 3.1415926;

        // ���µ���Ϸ����
        Units m_alive_self_units;
        Units m_alive_enemy_units;
        Units m_all_alive_units;
        UnitTypeData m_marine;
        bool m_is_save = false;
        // ��̬��Ϸ����
        GameInfo m_game_info;

        // �㷨����
        int m_frames_per_deploy = 12;
        int m_population_size = 50;
        float m_mutation_rate = 0.03;
        float m_crossover_rate = 1;

        // �㷨����
        population m_population;
        std::vector<float> m_hurt_objective; // Ŀ�꣺�ҷ��ܵ����˺�
        std::vector<float> m_damage_objective; // Ŀ�꣺�з��ܵ����˺�
        basic_solution m_selected_solution;
    };
}


#endif // !RANDOM_BOT_H