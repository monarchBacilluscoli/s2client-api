#ifndef RANDOM_BOT_H
#define RANDOM_BOT_H

#include "sc2api/sc2_api.h"
#include <iostream>
#include <vector>

namespace sc2 {

    //// 根据观察状态生成解
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

    //// 根据观察状态生成解并控制解的数量
    //class random_search {
    //public:
    //    random_search();
    //    // 问题每隔n帧向该函数传递一个观察，并从该函数得到一个解
    //    void get_command(const ObservationInterface* ob);

    //    ~random_search();

    //private:
    //    // 根据观察值生成随机解
    //    std::vector<basic_solution> generate_random_solutions(const ObservationInterface* ob);
    //    // 根据评估值选择随机解
    //    void select_solution(std::vector<basic_solution> ss);

    //    basic_solution generate_random_solution(const ObservationInterface* ob);
    //    std::pair<float, float> generate_random_position_in_circle(float radius);
    //    int population_size = 10;
    //    int frames_per_deploy = 24;
    //};

    // 控制游戏进行
    class random_bot : public Agent {
    public:
        using command = std::tuple<const Unit*, Point2D, const Unit*>;
        using basic_solution = std::vector<command>;
        using population = std::vector<std::pair<basic_solution, float>>;

        //virtual void initialize(int deploy_solution);
        virtual void OnGameStart() final;
        virtual void OnStep() final; // 关键处理，控制游戏进行，控制算法进行
        virtual void OnUnitIdle(const Unit* unit) final;

    private:
        // 生成一组随机解
        void generate_random_solution();
        // 生成一组无移动攻击最近单位的解
        void generate_attack_nearest_without_move_solution();
        // 交叉: 随机选择解之中的一个单位指令位置进行交换
        std::vector<basic_solution> cross_over(const basic_solution& a, const basic_solution& b);
        // 变异: 变异位置
        basic_solution mutate(const basic_solution & s, float random_ratio_of_command, float random_range);
        
        // 评估当前所有解
        void evaluate_all_solutions();
        // 根据评估值选择评估值最高的解
        basic_solution select_solution();
        // 随机选择某个解
        basic_solution select_random_solution();
        // 部署解
        void deploy_solution(const basic_solution& s);

        // 评估我方输出伤害
        float evaluate_single_solution_attack(const basic_solution& s);
        // 评估我方受到伤害
        float evaluate_single_solution_hurt(const basic_solution& s);

        // 生成圆内部随机点
        Point2D generate_random_point_in_circle(float r);
        // 在地图上搜索某点某范围内敌人
        Units search_units_within_radius(const Point2D& p, Unit::Alliance a, float r);
        // 在地图上搜索距离某点最近的特定单位
        const Unit* search_nearest_unit_from_point(const Point2D& p, Unit::Alliance a, Filter f = {});

        // 选择hp最少的敌人
        const Unit* select_lowest_hp_enemy(const Units& us);
        // 在给定单位群中搜索距离点最近的敌人
        const Unit* select_nearest_unit(const Point2D& p,const Units& us);
        
        // 计算武器对于某类型单位的伤害
        float damage_weapon_to_unit(const Weapon& w, const Unit* u);

        /* Debug所需函数 */
        // 显示武器范围
        void display_units_weapon_range(Units all_units);
        // 显示碰撞范围
        void display_units_collision_range(Units us);
        // 显示单位坐标
        void display_units_pos(Units us);
        // 显示地图坐标网格
        void display_map_grid(const GameInfo& info);
        // 显示单位动作
        void display_units_actions();
        // 显示单位移动
        void display_units_move_action(Units us);
        // 显示单位攻击
        void display_units_attack_aciton(Units us);
        // 显示解的路线和目标
        void display_solution_line(const basic_solution& bs);

        // 常量
        const float PI = 3.1415926;

        // 更新的游戏数据
        Units m_alive_self_units;
        Units m_alive_enemy_units;
        Units m_all_alive_units;
        UnitTypeData m_marine;
        bool m_is_save = false;
        // 静态游戏数据
        GameInfo m_game_info;

        // 算法设置
        int m_frames_per_deploy = 12;
        int m_population_size = 50;
        float m_mutation_rate = 0.03;
        float m_crossover_rate = 1;

        // 算法内容
        population m_population;
        std::vector<float> m_hurt_objective; // 目标：我方受到的伤害
        std::vector<float> m_damage_objective; // 目标：敌方受到的伤害
        basic_solution m_selected_solution;
    };
}


#endif // !RANDOM_BOT_H