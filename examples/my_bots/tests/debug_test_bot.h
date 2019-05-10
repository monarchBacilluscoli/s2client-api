// This bot is used to test Debug interface, especially those funcitons which can move/remove units and set the units' properties to make sure it can be used for the simulator
//? And the most important: Can I use them in Multi-player game? Wow, a bad feeling comes to me...
#ifndef DEBUG_TEST_BOT_H
#define DEBUG_TEST_BOT_H

#include<sc2api/sc2_api.h>
#include<iostream>

namespace sc2 {
    class debug_test_bot : public Agent
    {
    public:
        debug_test_bot() = default;
        ~debug_test_bot() = default;

        virtual void OnGameStart() final {
            m_game_info = Observation()->GetGameInfo();
            m_self_units = Observation()->GetUnits(Unit::Alliance::Self);
            m_enemy_units = Observation()->GetUnits(Unit::Alliance::Enemy);
            Units all_units = Observation()->GetUnits();
            //Kill all units
            for (const Unit* u: all_units)
            {
                Debug()->DebugKillUnit(u);
            }
            //todo create new units and move them into two different points
            Debug()->DebugCreateUnit(UNIT_TYPEID::TERRAN_MARINE, Point2D(m_game_info.playable_min.x, m_game_info.playable_min.y), 1, 10);
            Debug()->DebugCreateUnit(UNIT_TYPEID::TERRAN_MULE, Point2D(m_game_info.playable_max.x, m_game_info.playable_max.y), 2, 10);
            
            //todo save it
            //! no, Constrol->save() can not be used here, since it isn't useable in multi-player game.
            
            //todo set their life as half of the present values
            
            //! sentence to death and reborn, accept the destiny...
            Debug()->SendDebug();

            //? If I GetUnits(), what will happen?
            all_units = Observation()->GetUnits();
            for (const Unit* u : all_units)
            {
                Debug()->DebugKillUnit(u);
            }
            //! nothing will happen
        };
        virtual void OnStep() final {
            //todo get the difference between present step and the start step, base on it to set the health.
            std::cout<<Observation()->GetGameLoop()<<std::endl;
            if (Observation()->GetGameLoop()>=1) {
                hp_set_flag = true;
                //todo set the health
                Units all_units = Observation()->GetUnits();
                for (const Unit* u : all_units) {
                    Debug()->DebugSetLife(u->health / 2, u);
                }
                Debug()->SendDebug();
            }
        };
    private:
        Units m_self_units;
        Units m_enemy_units;
        GameInfo m_game_info;
        bool hp_set_flag = false;
    };
}

#endif // !DEBUG_TEST_BOT_H
