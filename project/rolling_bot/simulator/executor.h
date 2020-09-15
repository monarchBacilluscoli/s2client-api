#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "../../global_defines.h"

#include <limits>
#include <iostream>
#include <list>
#include <queue>
#include <map>
#include <list>

#include <sc2api/sc2_api.h>

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

    std::map<Tag, Unit> m_units_states_last_loop;          // for comparasion
    std::map<Tag, UnitStatisticalData> m_units_statistics; //
    std::map<Tag, Unit> m_initial_units_states;            // use for the comparision with current state
    std::map<Tag, const Unit *> m_initial_units;           // save all the initial units here or it will not be gotten if dead
    UnitsByAlliance m_dead_units;                          // for easy use

    bool m_is_setting = true; // if in setting state, do not call any of the client event about executing actions here
    bool m_last_is_setting = true;
    u_int32_t m_start_loop = 0;
    u_int32_t m_end_loop = std::numeric_limits<u_int32_t>::max();

    std::map<Tag, bool> m_is_an_attack_to_be_recorded;

    const float min_facing_change = 0.1745;

public:
    // transform vector commands to map+queue commands for easy use
    void SetCommands(const std::vector<Command> &commands);
    void Clear();      // clear all data for next simulation
    void Initialize(); // according to the new state initialize the records
    void SetIsSetting(bool is_setting);

    void OnStep() override;
    void OnUnitDestroyed(const Unit *unit) override;
    void OnUnitCreated(const Unit *unit) override;

    const std::list<Unit> &GetDeadUnits(Unit::Alliance alliance) const;
    const std::map<Tag, UnitStatisticalData> &GetUnitsStatistics();
    const UnitStatisticalData &GetUnitStatistics(Tag tag);
    GameResult CheckGameResult() const; // it may not be usefun when war fog existing
    u_int32_t GetEndLoop() const; // it may not be usefun when war fog existing

private:
    void RecordLastUnits();

    void ClearCommands();
    void ClearUnitsData();
    void ClearDeadUnits();
    void InitUnitStatistics(const Units &all_units);
    void RecordInitialUnitsStates(const Units &all_units);
    float CalculateHealthChange(Tag tag) const; // final - initial
};
} // namespace sc2

#endif // EXECUTOR_H
