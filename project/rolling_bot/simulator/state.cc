#include "state.h"
#include <sc2utils/sc2_manage_process.h>
#include <iostream>
#include "sc2lib/sc2_utils.h"

using namespace sc2;

bool UnitState::operator==(const UnitState &state) const
{
    // UnitTypeID unit_type;
    // Point2D pos;
    // uint32_t player_id = 0;
    // Tag unit_tag = 0; // only used in simulation
    // float energy = 0;
    // float life = 0;
    // float shield = 0;
    if (
        unit_type == state.unit_type &&
        pos == state.pos &&
        player_id == state.player_id &&
        unit_tag == state.unit_tag &&
        std::abs(energy - state.energy) <= 0.0001f &&
        std::abs(life - state.life) <= 0.0001f &&
        std::abs(shield - state.shield) <= 0.0001f)
    {
        return true;
    }
    return false;
}

bool UnitState::operator!=(const UnitState &state) const
{
    return !(*this == state);
}

bool State::operator==(const State &state) const
{
    return state.unit_states == unit_states;
}

bool State::operator!=(const State &state) const
{
    return !(*this == state);
}

State sc2::SaveMultiPlayerGame(const ObservationInterface *observation)
{
    State save;
    Units us = observation->GetUnits();
    save.unit_states.resize(us.size());
    const Unit *u;
    for (size_t i = 0; i < us.size(); i++)
    {
        u = us.at(i);
        save.unit_states[i] = UnitState({u->unit_type,
                                         u->pos,
                                         static_cast<uint32_t>(u->owner == 16 ? 0 : u->owner), // a inconsistency between editor player property & api property
                                         u->tag,                                               // this is just used for remote simulation
                                         u->energy,
                                         u->health,
                                         u->shield});
    }
    if (save.unit_states.size() < us.size())
    {
        std::cout << "mistake between save an us: " << us.size() - save.unit_states.size() << std::endl;
    }
    return save;
}

std::map<Tag, const Unit *> sc2::LoadMultiPlayerGame(State save, Client &current_client, Coordinator &current_coordinator)
{
    //todo consider the condition where the step size has been set to a value larger than 1
    std::map<Tag, const Unit *> unit_map;
    // kills all current units
    Units us = current_client.Observation()->GetUnits();
    current_client.Debug()->DebugKillUnits(us);
    current_client.Debug()->SendDebug();
    int step_size = current_coordinator.GetStepSize();
    int count = ceil(20 / step_size); // the wrekages need about 20 game loops to be cleaned
    for (size_t i = 0; i < count; i++)
    {
        current_coordinator.Update();
    }
    for (UnitState state_u : save.unit_states) // creates units from save
    {
        current_client.Debug()->DebugCreateUnit(state_u.unit_type, state_u.pos, state_u.player_id);
    }
    current_client.Debug()->SendDebug();
    count = ceil(3 / step_size); // the creation may need 3 (or 2?) loops to take effect (not so sure)
    for (size_t i = 0; i < count; ++i)
    {
        current_coordinator.Update();
    }
    const Unit *u_copied;
    Units us_copied = current_client.Observation()->GetUnits(); // just copys the units status in save
    for (const UnitState &state_u : save.unit_states)
    {
        unit_map[state_u.unit_tag] = u_copied = FindNearestUnitFromPoint(state_u.pos, us_copied);
        current_client.Debug()->DebugSetShields(state_u.shield + 0.1f, u_copied); // WTF, shield cannot be set to 0, if you set 0, you will find it is full in game, but if you set it as 0.1f, the data in game will be 0, fuck again.
        current_client.Debug()->DebugSetLife(state_u.life, u_copied);
        current_client.Debug()->DebugSetEnergy(state_u.energy, u_copied);
    }
    std::set<const Unit *> unit_set_of_map;
    for (std::pair<Tag, const Unit *> u : unit_map)
    {
        unit_set_of_map.insert(u.second);
    }
    if (unit_set_of_map.size() - us_copied.size() != 0)
    {
        std::cout << "load mistake!" << std::endl;
    }
    current_client.Debug()->SendDebug();
    count = ceil(2 / step_size);
    for (size_t i = 0; i < count; i++)
    {
        current_coordinator.Update();
    }

    return unit_map;
}