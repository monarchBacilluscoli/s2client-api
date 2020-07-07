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
    class Event
    {
        uint32_t m_game_loop;
        Point3D m_pos;

    public:
        uint32_t gameLoop() const { return m_game_loop; };
        Point3D position() const { return m_pos; };
        Event() : m_game_loop(0), m_pos(){};
        Event(uint32_t game_loop, const Point3D &pos) : m_game_loop(game_loop), m_pos(pos){};
    };
    class Action : public Event
    {
        AbilityID m_ability;

    public:
        AbilityID ability() const { return m_ability; };
        void setAbility(const ABILITY_ID &id) { m_ability = id; };
        Action() : Event(), m_ability(0) {}
        Action(uint32_t loop, const Point3D &pos, AbilityID ability) : Event(loop, pos), m_ability(ability) {}
    };
    class State : public Event
    {
        float m_value; // shield/health
    public:
        float value() const { return m_value; };
        State() : Event(), m_value(0.f){};
        State(uint32_t loop, const Point3D &pos, float state) : Event(loop, pos), m_value(state){};
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