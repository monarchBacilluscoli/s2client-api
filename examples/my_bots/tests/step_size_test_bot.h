// This is a test bot based on a single unit map, the ambition is to test pushing back and the effect of step size setting
#ifndef STEP_SIZE_TEST_BOT_H
#define STEP_SIZE_TEST_BOT_H

#include<sc2api/sc2_api.h>
#include<iostream>

namespace sc2 {
    class step_size_test_bot: public Agent
    {
    public:
        step_size_test_bot() = default;
        ~step_size_test_bot() = default;

        virtual void OnGameStart() final {
            m_only_runner = Observation()->GetUnits().front();
            m_game_info = Observation()->GetGameInfo();


            // test pushing back actions to the action list for a unit
            for (size_t i = 0; i < 10; i++)
            {
                // set random movement target step by step for the only_runner
                int x = GetRandomInteger(m_game_info.playable_min.x, m_game_info.playable_max.x);
                int y = GetRandomInteger(m_game_info.playable_min.y, m_game_info.playable_max.y);
                // display it in CLI
                std::cout << x << "," << y << std::endl;
                // move
                Actions()->UnitCommand(m_only_runner, ABILITY_ID::MOVE, Point2D(x, y),true);
            }
        };
        virtual void OnStep() final {
            //// set random movement target step by step for the only_runner
            //int x = GetRandomInteger(m_game_info.playable_min.x, m_game_info.playable_max.x);
            //int y = GetRandomInteger(m_game_info.playable_min.y, m_game_info.playable_max.y);
            //// display it in CLI
            //std::cout << x << "," << y << std::endl;
            //// move
            //Actions()->UnitCommand(m_only_runner, ABILITY_ID::MOVE, Point2D(x, y));
        };
    private:
        const Unit* m_only_runner;
        GameInfo m_game_info;
    };
}

#endif // !STEP_SIZE_TEST_BOT_H
