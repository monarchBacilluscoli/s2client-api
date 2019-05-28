#ifndef POTENTIAL_FIELD_BOT
#define POTENTIAL_FIELD_BOT

#include<iostream>
#include<sc2api/sc2_api.h>

namespace sc2 {
    class potential_field_bot:public Agent {
    public:
        //! 初始化一些不需要了解的参数
        potential_field_bot(int step_size = 1):m_step_size(step_size) {};
        virtual ~potential_field_bot() = default;

        //! 计算一群单位对某个单位的合力
        virtual Vector2D force_to_unit(const Units& sources, const Unit* u);

        //! 在游戏开始时自动被调用一次，做一些初始设定等等
        virtual void OnGameStart();
        //! 在游戏的每一帧更新时被自动调用一次，你需要控制你的单位做什么事情就都写在这个函数里了
        virtual void OnStep();

    protected:
        //! 给你传进去的unit找到离他最近的家伙用于攻击
        virtual const Unit* select_target_enemy(const Unit* unit);

        //! this is copied from another bot directly.
        //! 工具函数
        const Unit* select_nearest_unit_from_point(const Point2D& p, const Units& us);
        //todo search units can be attacked

        //! 计算某个敌方unit对传进去unit的力
        virtual Vector2D force_enemy_to_unit(const Unit* source, const Unit* target);
        //! 计算某个我方#%￥%……#
        virtual Vector2D force_ally_to_unit(const Unit* source, const Unit* target);
        //! 计算墙#%￥……&#@
        virtual Vector2D force_wall_to_unit(const Unit* target);

        //! 计算某个敌方unit对传进去unit的引力
        virtual float calculate_enemy_attraction_value(const Unit* enemy, const Unit* unit) const { return 0.f; };
        //! 计算某个敌方……（*&（*&）—斥力
        virtual float calculate_enemy_repulsion_value(const Unit* enemy, const Unit* unit) const { return 0.f; };
        //! 计算我方￥&……%引力
        virtual float calculate_ally_attraction_value(const Unit* ally, const Unit* unit) const { return 0.f; };
        //! 计算我方%&……%斥力
        virtual float calculate_ally_repulsion_value(const Unit* ally, const Unit* unit) const { return 0.f; };

        // 工具函数，用于计算力
        float calculate_enemy_zero_field_dis(const Unit* source, const Unit* target);
        float calculate_ally_zero_field_dis(const Unit* source, const Unit* target);

        //! 用于计算在受力情况下一个step内单位的移动距离（用于根据力控制单位移动）
        float move_dis_per_time_slice(const Unit* u) const;

        // 工具函数
        Units serach_enemies_can_be_attacked_by_unit(const Unit* u);

        // 工具函数，用于显示力场情况
        void display_facing_direction(const Units& us, DebugInterface* debug);
        void display_force_direction(const Unit* u, const Vector2D& force,DebugInterface* debug);
        void display_field_boundary(const Unit* selected_u, const Units& source_us, DebugInterface* debug);
        void display_fire_range(const Units& us, DebugInterface* debug);
        void display_playable_area(const GameInfo& gi, DebugInterface* debug);

        //! 一些简单的参数
        const float m_zero_potential_energy_ratio = 0.8f;
        const float m_zero_ally_field_ratio = 0.6f;
        const float m_wall_force_factor = 10; // must be big enough

        //! 一些游戏运行中的数据
        Units m_alive_self_units;
        Units m_alive_enemy_units;
        Units m_all_alive_units;
        UnitTypes m_unit_types;
        GameInfo m_game_info;

        /*
         * some additional data members used in derived class
        */
        //! 另一些呃...游戏运行中数据
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
