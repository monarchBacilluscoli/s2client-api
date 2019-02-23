#pragma once

#include "sc2api/sc2_api.h"
#include <iostream>

namespace sc2 {
    class Bot : public Agent { // Agent�̳���������ݣ������鿴���ҿ�����Ϸ״̬���Լ���Bot�̳�Agent�Ǳ�Ҫ��
    public:
        virtual void OnGameStart() final;
        virtual void OnUnitCreated(const Unit* unit);
        // OnStep()��������ÿ�ν���coordinator.Update()��ʱ�򱻵���
        virtual void OnStep() final;
        virtual void OnGameEnd() final;
        // ������е�λ
        virtual void OnUnitIdle(const Unit* unit) final;
        // �����ҷ�ĳ�ֵ�λ�����������ã�
        size_t CountUnitType(UNIT_TYPEID unit_type);
        // ���ƼӸǽ�������
        bool TryBuiltStructure(ABILITY_ID ability_type_for_structure, UNIT_TYPEID unit_type = UNIT_TYPEID::TERRAN_SCV);
        // ���Խ��첹��վ
        bool TryBuildSupplyDepot();
        // ���Խ��쾫����
        bool TryBuildRefinery();
        // ���Խ����Ӫ
        bool TryBuildBarracks();
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
        const Unit* FindNearestResource(const Point2D& start, UNIT_TYPEID resource_type = UNIT_TYPEID::NEUTRAL_MINERALFIELD);
        // Ѱ�����������λ
        const Unit* FindNearestBuilding(const Point2D& start, UNIT_TYPEID building_type = UNIT_TYPEID::TERRAN_REFINERY);

    private:
        const float m_resources_exploite_probablity = 0.7f; // ����SCV����ʱ�ɼ��������˹�ı���
    };
}

