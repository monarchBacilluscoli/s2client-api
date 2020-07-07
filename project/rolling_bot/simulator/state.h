// It is the record of the state at a point of time during a game, used as a save for Control()->Save() doesn't function in multiplyer game.

#ifndef STATE_H
#define STATE_H

#include <sc2api/sc2_api.h>
#include <map>

namespace sc2
{
struct UnitState
{
    UnitTypeID unit_type;
    Point2D pos;
    uint32_t player_id = 0;
    Tag unit_tag = 0; // only used in simulation
    float energy = 0;
    float life = 0;
    float shield = 0;

    bool operator==(const UnitState &state) const;
    bool operator!=(const UnitState &state) const;
};

struct State
{
    std::vector<UnitState> unit_states;
    bool operator==(const State &state) const;
    bool operator!=(const State &state) const;
};

//! Save the current game status to a State object
//! note that you'd better not set the step size as any number that larger than 1, or this load function will not function properly...
//! \param observation The Observation object which can observe current game, the client it belongs to should have the whole view of the map, or it will be meaningless
State SaveMultiPlayerGame(const ObservationInterface *observation);
//! Loads a game state from a State object
//! \param save The State object which save the game status which needs to be loaded
//! \param client It is used to set units, so it must be the current client
//! \param coordinator It is used to set units, too, so it should also be the coordinator of the current game
//! \return returns the map from original units to created units
std::map<Tag, const Unit *> LoadMultiPlayerGame(State save, Client &current_client, Coordinator &current_coordinator);
} // namespace sc2

#endif //STATE_H