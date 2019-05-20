#ifndef STATE_H
#define STATE_H

#include<sc2api/sc2_api.h>

namespace sc2 {
    struct UnitState {
        UnitTypeID unit_type;
        Point2D pos;
        uint32_t player_id = 0;
        Tag unit_tag = 0; // only used in simulation
        float energy = 0;
        float life = 0;
        float shields = 0;
    };

    struct State
    {
        std::vector<UnitState> unit_states;
    };

    //todo save & load in multi-player game
    //? those are utility functions, so don't need to wrap them into a class
    State SaveMultiPlayerGame(const ObservationInterface* observation);
    //todo loads a game state from a State object, need to use Debug() to set  and Control()
    void LoadMultiPlayerGame(State save, Client& client, Coordinator& coordinator);
}

#endif // !STATE_H

