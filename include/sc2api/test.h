// Added by LiuYongfeng for easy test

#include "global_defines.h"

#include <string>
#include <iostream>
#include <chrono>
#include <vector>
#include <sc2api/sc2_api.h>

#define CORS_COUNT 100

namespace sc2
{
    class ANBot : public Agent
    {
        void OnGameEnd() final
        {
            // AgentControl()->Restart();
        }

        void OnUnitIdle(const Unit *u) final
        {
            Units enemies = Observation()->GetUnits(Unit::Alliance::Enemy);
            Actions()->UnitCommand(u, ABILITY_ID::ATTACK, enemies.front());
        }
    };

    class Test
    {
    private:
    public:
        Test(/* args */) = default;
        ~Test() = default;

        static void Update(Coordinator &cor, int frames)
        {
            for (int i = 0; i < frames; ++i)
            {
                cor.Update();
            }
        }

        static void CSetAndStart(Coordinator &cor, int port, int argc, char *argv[], Agent *bot1, Agent *bot2)
        {
            cor.LoadSettings(argc, argv);
            cor.SetParticipants({CreateParticipant(Terran, bot1), CreateParticipant(Terran, bot2)});
            cor.SetPortStart(port);
            cor.SetMapPath("PCANP_EnemyZealotModVSMarines.SC2Map");
            cor.SetStepSize(1);
            cor.LaunchStarcraft();
            cor.StartGame();
        }

        static void SetAndStartCors(int argc, char *argv[])
        {
            std::fstream file("./time.txt", std::ios::app);

            std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

            std::vector<Coordinator> cors(CORS_COUNT);
            ANBot bots1[CORS_COUNT];
            ANBot bots2[CORS_COUNT];
            bool one_by_one = false;
            std::vector<std::future<void>> cors_futures(CORS_COUNT);
            int count = CORS_COUNT;
            int batch_count = 10;

            if (one_by_one)
            {
                for (int i = 0; i < count; ++i)
                {
                    CSetAndStart(cors[i], 4000 + i * 10, argc, argv, &(bots1[i]), &(bots2[i]));
                }
            }
            else
            {
                for (int i = 0; i < count; ++i)
                {
                    cors_futures[i] = std::async(std::launch::async, &Test::CSetAndStart, std::ref(cors[i]), 4000 + 10 * i, argc, argv, &(bots1[i]), &(bots2[i]));
                    if (i != 0 && i % batch_count == 0)
                    {
                        for (int j = i - batch_count; j <= i; ++j) // wait on every 20 launches
                        {
                            cors_futures[j].wait();
                        }
                    }
                }
            }

            std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
            std::chrono::steady_clock::duration time_used = end - start;
            file << "launch:\t" << (std::chrono::duration_cast<std::chrono::milliseconds>(time_used)).count() << std::endl;

            for (int i = 0; i < count; ++i)
            {
                cors_futures[i] = std::async(&Test::Update, std::ref(cors[i]), 10);
            }
            for (int i = 0; i < count; ++i)
            {
                cors_futures[i].wait();
            }

            std::chrono::steady_clock::time_point update_end = std::chrono::steady_clock::now();
            file << "update:\t" << (std::chrono::duration_cast<std::chrono::milliseconds>(update_end - end)).count() << std::endl;
            return;
        }
    };
} // namespace sc2
