#include "bot_tutor.h"

namespace sc2 {
    void Bot::OnGameStart() { // 这些个内容在ClentEvents里
        std::cout << "Hello, World!" << std::endl;
        std::cout << "Map Width: " << Observation()->GetGameInfo().width << std::endl;
    }
    void Bot::OnUnitCreated(const Unit * unit) {
        std::cout << unit->tag << std::endl;
    }

    // OnStep()方法将在每次进行coordinator.Update()的时候被调用
    void Bot::OnStep() {
        if (Observation()->GetGameLoop() % 50 == 0) { // 每隔50个loop显示一次内容
            std::cout << Observation()->GetGameLoop() << std::endl; // 显示游戏循环的数目
            std::cout << "Minerals: " << Observation()->GetMinerals() << std::endl; // 显示矿物数量
            std::cout << "Vespene: " << Observation()->GetVespene() << std::endl; // 显示瓦斯数量
        }

        // 尝试建造补给站，如果当前step不能加盖成功，那就下一step继续加盖即可
        TryBuildSupplyDepot();
        // 如果精炼厂不够，则加盖精炼厂
        const Units units = Observation()->GetUnits(Unit::Alliance::Self);
        int refinery_count = 0;
        for (const auto& u : units) {
            if (u->unit_type == UNIT_TYPEID::TERRAN_REFINERY) {
                refinery_count++;
            }
        }
        if (refinery_count < 2) {
            TryBuildRefinery();
        }
        // 尝试建造兵营
        TryBuildBarracks();
    }
    void Bot::OnGameEnd() {
        std::cout << "result: " << Observation()->GetResults()[0].result << std::endl;
        system("pause");
    }

    // 处理空闲单位
    void Bot::OnUnitIdle(const Unit * unit) {
        switch (unit->unit_type.ToType()) { // 将对象的类型显式转化为枚举类型
        case UNIT_TYPEID::TERRAN_COMMANDCENTER: {
            if (CountUnitType(UNIT_TYPEID::TERRAN_SCV) < 20) {
                Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_SCV); // 部署动作（单位，动作），在游戏中无论是建筑还是单位都是Unit
            }
            break;
        }
        case UNIT_TYPEID::TERRAN_SCV: {
            // 如果有闲置scv，则使用它进行采矿（随机采集晶矿和瓦斯）
            if (GetRandomFraction() < m_resources_exploite_probablity) {
                // 采集瓦斯
                const Unit* refinery_target = FindNearestBuilding(unit->pos);
                Actions()->UnitCommand(unit, ABILITY_ID::SMART, refinery_target);
            }
            else {
                // 采集晶矿
                const Unit* mineral_target = FindNearestResource(unit->pos);
                Actions()->UnitCommand(unit, ABILITY_ID::SMART, mineral_target);
                break;
            }
        }
        case UNIT_TYPEID::TERRAN_BARRACKS: {
            Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_MARINE);
            break;
        }
        case UNIT_TYPEID::TERRAN_MARINE: {
            //const GameInfo& game_info = Observation()->GetGameInfo(); // 得到敌方起始位置
            float rx = GetRandomScalar(); // 获取单个随机数
            float ry = GetRandomScalar();
            Point2D barracks_pos = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_BARRACKS)).front()->pos;
            Point2D location(barracks_pos.x + 5 * rx, barracks_pos.y + 5 * ry);
            Actions()->UnitCommand(unit, ABILITY_ID::MOVE, location);
            /*const Unit* target_mineral = FindNearestResource(unit->pos);
            Actions()->UnitCommand(unit, ABILITY_ID::MOVE, target_mineral->pos);*/
            break;
        }
        default:
            break;
        }
    }

    // 返回我方某种单位数量（很有用）
    size_t Bot::CountUnitType(UNIT_TYPEID unit_type) {
        return Observation()->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
    }

    // 控制加盖建筑功能
    bool Bot::TryBuiltStructure(ABILITY_ID ability_type_for_structure, UNIT_TYPEID unit_type) {
        const ObservationInterface* observation = Observation(); // 得到一个Observation对象用于...
                                                                 // 如果存在单位正在加盖该类建筑，则不进行加盖，反之则进行加盖
        const Unit* unit_to_build = nullptr; // 嗯，这是指针常量，注意const修饰的是Unit，也就是说这是一个指向const Unit的指针
        Units units = observation->GetUnits(Unit::Alliance::Self);
        for (const auto& unit : units) {
            for (const auto& order : unit->orders) {
                // 判断unit的当前指令是否是加盖该种建筑
                if (order.ability_id == ability_type_for_structure) {
                    return false;
                }
            }
            if (unit->unit_type == unit_type) {
                unit_to_build = unit;
                // 此处不能break，如果后面有已经正在造东西的unit，那么那时才要break
            }
        }
        float rx = GetRandomScalar(); // 返回-1/1的随机数
        float ry = GetRandomScalar();

        //TODO: 此处报错
        Actions()->UnitCommand(unit_to_build, ability_type_for_structure, Point2D(unit_to_build->pos.x + rx * 15.0f, unit_to_build->pos.y + ry * 15.0f));

        return true;
    }

    // 尝试建造补给站
    bool Bot::TryBuildSupplyDepot() {
        const ObservationInterface* observation = Observation(); // 这一句稍稍有些不理解
                                                                 // 如果食物还有两个或以上余量，则不需要加盖补给站；反之需要
        if (observation->GetFoodUsed() <= observation->GetFoodCap() - 2) {
            return false;
        }
        else {
            // 调用建造建筑函数输入supply并返回
            return TryBuiltStructure(ABILITY_ID::BUILD_SUPPLYDEPOT);
        }
    }

    // 尝试建造精炼厂
    bool Bot::TryBuildRefinery() {
        const ObservationInterface* observation = Observation(); // 得到一个Observation对象用于...
                                                                 // 如果存在SCV正在加盖该类建筑，则不进行加盖，反之则进行加盖
        Units units = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_SCV));
        for (const auto& unit : units) {
            for (const auto& order : unit->orders) {
                // 判断unit的当前指令是否是加盖该种建筑
                if (order.ability_id == ABILITY_ID::BUILD_REFINERY) {
                    return false;
                }
            }
            // 寻找附近的天然气位置并进行加盖
            const Unit* target = FindNearestResource(unit->pos, UNIT_TYPEID::NEUTRAL_VESPENEGEYSER);
            Actions()->UnitCommand(unit, ABILITY_ID::BUILD_REFINERY, target);
            return true;
        }
        // 如果在循环里没有return，那么就是没造出来
        return false;
    }

    // 尝试建造兵营
    bool Bot::TryBuildBarracks() {
        // 如果没有补给站，就不建兵营（兵营依赖补给站）
        if (CountUnitType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT) < 1) {
            return false;
        }
        // 有一个兵营也就够了，不建
        if (CountUnitType(UNIT_TYPEID::TERRAN_BARRACKS) > 0) {
            return false;
        }
        return TryBuiltStructure(ABILITY_ID::BUILD_BARRACKS);
    }

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
    const Unit * Bot::FindNearestResource(const Point2D & start, UNIT_TYPEID resource_type) {
        const Units units = Observation()->GetUnits(Unit::Alliance::Neutral); // 得到所有Neutral（中立）单位（比如矿堆）
        float minDis = std::numeric_limits<float>::max(); // 忘了静态函数可以通过类名直接调用，注意numeric_limits的用法
        const Unit* target = nullptr;
        // 通过循环寻找距离最小值所属的资源
        for (const auto& unit : units) {
            if (unit->unit_type == resource_type) { // 遍历是否为所需资源
                if (Distance2D(start, unit->pos) < minDis) {
                    minDis = Distance2D(start, unit->pos);
                    target = unit;
                }
            }
        }
        return target;
    }

    // 寻找最近建筑单位
    inline const Unit * Bot::FindNearestBuilding(const Point2D & start, UNIT_TYPEID building_type) {
        const Units units = Observation()->GetUnits(Unit::Alliance::Self); // 得到所有我方单位（TODO:此处应使用lambda表达式进行过滤）
        float minDis = std::numeric_limits<float>::max(); // 忘了静态函数可以通过类名直接调用，注意numeric_limits的用法
        const Unit* target = nullptr;
        // 通过循环寻找距离最小值所属的资源
        for (const auto& unit : units) {
            if (unit->unit_type == building_type) { // 遍历是否为所需资源（TODO: 可以写在GetUnits里面）
                if (Distance2D(start, unit->pos) < minDis) {
                    minDis = Distance2D(start, unit->pos);
                    target = unit;
                }
            }
        }
        return target;
    }
}