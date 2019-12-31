#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <sc2api/sc2_api.h>
#include <iostream>
#include <list>
#include <queue>
#include <list>
#include <map>
#include "statistical_data.h"
#include "command.h"

namespace sc2
{
struct UnitsByAlliance
{
    std::list<Unit> self{}, ally{}, enemy{}, neutral{};
};

class Executor : public Agent // executor of actions and observer of game status
{
private:
    std::map<Tag, std::queue<ActionRaw>> m_commands;

    std::map<Tag, float> m_cooldown_last_frame;
    std::map<Tag, UnitStatisticalData> m_units_statistics;
    std::map<Tag, Unit> m_initial_units_states;
    std::map<Tag, const Unit *> m_initial_units;
    UnitsByAlliance m_dead_units;

    bool m_is_setting = true; // if in setting state, do not call any of the client event here

public:
    // transform vector commands to map+queue commands for easy use
    void SetCommands(const std::vector<Command> &commands);
    void Clear();
    void Initialize();
    void SetIsSetting(bool is_setting);

    void OnStep() override;
    void OnUnitDestroyed(const Unit *unit) override;
    void OnUnitCreated(const Unit *unit) override;

    const std::list<Unit> &GetDeadUnits(Unit::Alliance alliance) const;
    const std::map<Tag, UnitStatisticalData> &GetUnitsStatistics();
    const UnitStatisticalData& GetUnitStatistics(Tag tag);
    GameResult CheckGameResult() const; // it may not be usefun when war fog existing

private:
    void ClearCommands();
    void ClearUnitsData();
    void ClearCooldownData();
    void ClearDeadUnits();
    void InitUnitStatistics(const Units &all_units);
    void RecordInitialUnitsStates(const Units &all_units);
    float CalculateHealthChange(Tag tag) const;
};
} // namespace sc2

#endif // EXECUTOR_H
