#pragma once

#include "sc2api/sc2_api.h"
#include <iostream>

namespace sc2 {
    class Bot : public Agent { // Agent继承了诸多内容，如果想查看并且控制游戏状态，自己的Bot继承Agent是必要的
    public:
        virtual void OnGameStart() final;
        virtual void OnUnitCreated(const Unit* unit);
        // OnStep()方法将在每次进行coordinator.Update()的时候被调用
        virtual void OnStep() final;
        virtual void OnGameEnd() final;
        // 处理空闲单位
        virtual void OnUnitIdle(const Unit* unit) final;
        // 返回我方某种单位数量（很有用）
        size_t CountUnitType(UNIT_TYPEID unit_type);
        // 控制加盖建筑功能
        bool TryBuiltStructure(ABILITY_ID ability_type_for_structure, UNIT_TYPEID unit_type = UNIT_TYPEID::TERRAN_SCV);
        // 尝试建造补给站
        bool TryBuildSupplyDepot();
        // 尝试建造精炼厂
        bool TryBuildRefinery();
        // 尝试建造兵营
        bool TryBuildBarracks();
        //// 寻找附近矿堆，就是get到所有自然资源，然后筛选出矿堆，然后寻找相对距离最小的矿堆
        //const Unit* FindNearestMineralPatch(const Point2D& start) {
        //    const Units units = Observation()->GetUnits(Unit::Alliance::Neutral); // 得到所有Neutral（中立）单位，比如矿堆
        //    float minDis = std::numeric_limits<float>::max(); // 忘了静态函数可以通过类名直接调用，注意numeric_limits的用法
        //    const Unit* target = nullptr;
        //    for (const auto& unit : units) {
        //        if (unit->unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD) { // 外侧遍历是否矿堆
        //            if (Distance2D(start, unit->pos)<minDis) {
        //                minDis = Distance2D(start, unit->pos);
        //                target = unit;
        //            }
        //        }
        //    }
        //    return target;
        //}
        // 寻找最近所需资源
        const Unit* FindNearestResource(const Point2D& start, UNIT_TYPEID resource_type = UNIT_TYPEID::NEUTRAL_MINERALFIELD);
        // 寻找最近建筑单位
        const Unit* FindNearestBuilding(const Point2D& start, UNIT_TYPEID building_type = UNIT_TYPEID::TERRAN_REFINERY);

    private:
        const float m_resources_exploite_probablity = 0.7f; // 控制SCV闲置时采集晶矿和瓦斯的比例
    };
}

