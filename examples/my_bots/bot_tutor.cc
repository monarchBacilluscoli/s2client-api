#include "bot_tutor.h"

namespace sc2 {
    void Bot::OnGameStart() { // ��Щ��������ClentEvents��
        std::cout << "Hello, World!" << std::endl;
        std::cout << "Map Width: " << Observation()->GetGameInfo().width << std::endl;
    }
    void Bot::OnUnitCreated(const Unit * unit) {
        std::cout << unit->tag << std::endl;
    }

    // OnStep()��������ÿ�ν���coordinator.Update()��ʱ�򱻵���
    void Bot::OnStep() {
        if (Observation()->GetGameLoop() % 50 == 0) { // ÿ��50��loop��ʾһ������
            std::cout << Observation()->GetGameLoop() << std::endl; // ��ʾ��Ϸѭ������Ŀ
            std::cout << "Minerals: " << Observation()->GetMinerals() << std::endl; // ��ʾ��������
            std::cout << "Vespene: " << Observation()->GetVespene() << std::endl; // ��ʾ��˹����
        }

        // ���Խ��첹��վ�������ǰstep���ܼӸǳɹ����Ǿ���һstep�����ӸǼ���
        TryBuildSupplyDepot();
        // �����������������ӸǾ�����
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
        // ���Խ����Ӫ
        TryBuildBarracks();
    }
    void Bot::OnGameEnd() {
        std::cout << "result: " << Observation()->GetResults()[0].result << std::endl;
        system("pause");
    }

    // ������е�λ
    void Bot::OnUnitIdle(const Unit * unit) {
        switch (unit->unit_type.ToType()) { // �������������ʽת��Ϊö������
        case UNIT_TYPEID::TERRAN_COMMANDCENTER: {
            if (CountUnitType(UNIT_TYPEID::TERRAN_SCV) < 20) {
                Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_SCV); // ����������λ��������������Ϸ�������ǽ������ǵ�λ����Unit
            }
            break;
        }
        case UNIT_TYPEID::TERRAN_SCV: {
            // ���������scv����ʹ�������вɿ�����ɼ��������˹��
            if (GetRandomFraction() < m_resources_exploite_probablity) {
                // �ɼ���˹
                const Unit* refinery_target = FindNearestBuilding(unit->pos);
                Actions()->UnitCommand(unit, ABILITY_ID::SMART, refinery_target);
            }
            else {
                // �ɼ�����
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
            //const GameInfo& game_info = Observation()->GetGameInfo(); // �õ��з���ʼλ��
            float rx = GetRandomScalar(); // ��ȡ���������
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

    // �����ҷ�ĳ�ֵ�λ�����������ã�
    size_t Bot::CountUnitType(UNIT_TYPEID unit_type) {
        return Observation()->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
    }

    // ���ƼӸǽ�������
    bool Bot::TryBuiltStructure(ABILITY_ID ability_type_for_structure, UNIT_TYPEID unit_type) {
        const ObservationInterface* observation = Observation(); // �õ�һ��Observation��������...
                                                                 // ������ڵ�λ���ڼӸǸ��ཨ�����򲻽��мӸǣ���֮����мӸ�
        const Unit* unit_to_build = nullptr; // �ţ�����ָ�볣����ע��const���ε���Unit��Ҳ����˵����һ��ָ��const Unit��ָ��
        Units units = observation->GetUnits(Unit::Alliance::Self);
        for (const auto& unit : units) {
            for (const auto& order : unit->orders) {
                // �ж�unit�ĵ�ǰָ���Ƿ��ǼӸǸ��ֽ���
                if (order.ability_id == ability_type_for_structure) {
                    return false;
                }
            }
            if (unit->unit_type == unit_type) {
                unit_to_build = unit;
                // �˴�����break������������Ѿ������춫����unit����ô��ʱ��Ҫbreak
            }
        }
        float rx = GetRandomScalar(); // ����-1/1�������
        float ry = GetRandomScalar();

        //TODO: �˴�����
        Actions()->UnitCommand(unit_to_build, ability_type_for_structure, Point2D(unit_to_build->pos.x + rx * 15.0f, unit_to_build->pos.y + ry * 15.0f));

        return true;
    }

    // ���Խ��첹��վ
    bool Bot::TryBuildSupplyDepot() {
        const ObservationInterface* observation = Observation(); // ��һ��������Щ�����
                                                                 // ���ʳ�ﻹ����������������������Ҫ�Ӹǲ���վ����֮��Ҫ
        if (observation->GetFoodUsed() <= observation->GetFoodCap() - 2) {
            return false;
        }
        else {
            // ���ý��콨����������supply������
            return TryBuiltStructure(ABILITY_ID::BUILD_SUPPLYDEPOT);
        }
    }

    // ���Խ��쾫����
    bool Bot::TryBuildRefinery() {
        const ObservationInterface* observation = Observation(); // �õ�һ��Observation��������...
                                                                 // �������SCV���ڼӸǸ��ཨ�����򲻽��мӸǣ���֮����мӸ�
        Units units = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_SCV));
        for (const auto& unit : units) {
            for (const auto& order : unit->orders) {
                // �ж�unit�ĵ�ǰָ���Ƿ��ǼӸǸ��ֽ���
                if (order.ability_id == ABILITY_ID::BUILD_REFINERY) {
                    return false;
                }
            }
            // Ѱ�Ҹ�������Ȼ��λ�ò����мӸ�
            const Unit* target = FindNearestResource(unit->pos, UNIT_TYPEID::NEUTRAL_VESPENEGEYSER);
            Actions()->UnitCommand(unit, ABILITY_ID::BUILD_REFINERY, target);
            return true;
        }
        // �����ѭ����û��return����ô����û�����
        return false;
    }

    // ���Խ����Ӫ
    bool Bot::TryBuildBarracks() {
        // ���û�в���վ���Ͳ�����Ӫ����Ӫ��������վ��
        if (CountUnitType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT) < 1) {
            return false;
        }
        // ��һ����ӪҲ�͹��ˣ�����
        if (CountUnitType(UNIT_TYPEID::TERRAN_BARRACKS) > 0) {
            return false;
        }
        return TryBuiltStructure(ABILITY_ID::BUILD_BARRACKS);
    }

    //// Ѱ�Ҹ�����ѣ�����get��������Ȼ��Դ��Ȼ��ɸѡ����ѣ�Ȼ��Ѱ����Ծ�����С�Ŀ��
    //const Unit* FindNearestMineralPatch(const Point2D& start) {
    //    const Units units = Observation()->GetUnits(Unit::Alliance::Neutral); // �õ�����Neutral����������λ��������
    //    float minDis = std::numeric_limits<float>::max(); // ���˾�̬��������ͨ������ֱ�ӵ��ã�ע��numeric_limits���÷�
    //    const Unit* target = nullptr;
    //    for (const auto& unit : units) {
    //        if (unit->unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD) { // �������Ƿ���
    //            if (Distance2D(start, unit->pos)<minDis) {
    //                minDis = Distance2D(start, unit->pos);
    //                target = unit;
    //            }
    //        }
    //    }
    //    return target;
    //}
    // Ѱ�����������Դ
    const Unit * Bot::FindNearestResource(const Point2D & start, UNIT_TYPEID resource_type) {
        const Units units = Observation()->GetUnits(Unit::Alliance::Neutral); // �õ�����Neutral����������λ�������ѣ�
        float minDis = std::numeric_limits<float>::max(); // ���˾�̬��������ͨ������ֱ�ӵ��ã�ע��numeric_limits���÷�
        const Unit* target = nullptr;
        // ͨ��ѭ��Ѱ�Ҿ�����Сֵ��������Դ
        for (const auto& unit : units) {
            if (unit->unit_type == resource_type) { // �����Ƿ�Ϊ������Դ
                if (Distance2D(start, unit->pos) < minDis) {
                    minDis = Distance2D(start, unit->pos);
                    target = unit;
                }
            }
        }
        return target;
    }

    // Ѱ�����������λ
    inline const Unit * Bot::FindNearestBuilding(const Point2D & start, UNIT_TYPEID building_type) {
        const Units units = Observation()->GetUnits(Unit::Alliance::Self); // �õ������ҷ���λ��TODO:�˴�Ӧʹ��lambda���ʽ���й��ˣ�
        float minDis = std::numeric_limits<float>::max(); // ���˾�̬��������ͨ������ֱ�ӵ��ã�ע��numeric_limits���÷�
        const Unit* target = nullptr;
        // ͨ��ѭ��Ѱ�Ҿ�����Сֵ��������Դ
        for (const auto& unit : units) {
            if (unit->unit_type == building_type) { // �����Ƿ�Ϊ������Դ��TODO: ����д��GetUnits���棩
                if (Distance2D(start, unit->pos) < minDis) {
                    minDis = Distance2D(start, unit->pos);
                    target = unit;
                }
            }
        }
        return target;
    }
}