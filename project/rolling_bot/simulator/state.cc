#include "state.h"
#include <sc2utils/sc2_manage_process.h>
#include <iostream>
#include "sc2lib/sc2_utils.h"

using namespace sc2;

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
                                         static_cast<uint32_t>(u->owner),
                                         u->tag, // this is just used for remote simulation
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
    // the wrekages need about 20 game loops to be cleaned
    int step_size = current_coordinator.GetStepSize();
    int count = ceil(20/step_size);
    for (size_t i = 0; i < count; i++)
    {
        current_coordinator.Update();
    }
    // creates units from save
    for (UnitState state_u : save.unit_states)
    {
        current_client.Debug()->DebugCreateUnit(state_u.unit_type, state_u.pos, state_u.player_id);
    }
    current_client.Debug()->SendDebug();
    count = count = ceil(2 / step_size);
    for (size_t i = 0; i < count; i++)
    {
        current_coordinator.Update();
    }
    // just copys the units in save
    const Unit *u_copied;
    Units us_copied = current_client.Observation()->GetUnits();
    for (const UnitState &state_u : save.unit_states)
    {
        unit_map[state_u.unit_tag] = u_copied = SelectNearestUnitFromPoint(state_u.pos, us_copied);
        current_client.Debug()->DebugSetShields(state_u.shields + 0.1f, u_copied); // WTF, shield cannot be set to 0, if you set 0, you will find it is full in game, but if you set it as 0.1f, the data in game will be 0, fuck again.
        current_client.Debug()->DebugSetLife(state_u.life, u_copied);
        current_client.Debug()->DebugSetEnergy(state_u.energy, u_copied);
    }
    //! check the load results
    //todo get all the remaining units in save and unit_map, to see if the map is ok
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
    count = count = ceil(2 / step_size);
    for (size_t i = 0; i < count; i++)
    {
        current_coordinator.Update();
    }

    return unit_map;
}