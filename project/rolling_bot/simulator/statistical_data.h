//This file includes some events happen during the game play: actions the units take, states changes and so on.
//The structure of them is when and what
#ifndef STATISTICAL_DATA_H
#define STATISTICAL_DATA_H

#include <list>
#include <sc2api/sc2_action.h>

namespace sc2
{
struct Events
{
    struct Action
    {
        uint32_t game_loop;
        AbilityID ability;
        Action() : game_loop(0), ability(0) {}
        Action(uint32_t loop, AbilityID ab) : game_loop(loop), ability(ab) {}
    };
    struct State
    {
        uint32_t game_loop;
        float state; // shield/health
        State() : game_loop(0), state(0.f){};
        State(uint32_t loop, float st) : game_loop(loop), state(st){};
    };
    struct Position
    {
        uint32_t game_loop;
        Point2D position;
        Position() : game_loop(0), position(Point2D()){};
        Position(uint32_t loop, Point2D pos) : game_loop{loop}, position(pos){};
    };
    std::list<Action> actions; // only self unit
    std::list<State> health;
    std::list<State> shield;
    uint32_t dead_loop = 0;

    Events() = default;
};

// The class used to record a unit's data during a game
struct UnitStatisticalData
{
    // self unit's data
    int action_number = 0;
    int attack_number = 0;
    Events events = Events(); // record the action and state change of a unit
    // data of unit of all alliances
    float health_change = 0.f;

    UnitStatisticalData() = default;
};
} // namespace sc2

#endif