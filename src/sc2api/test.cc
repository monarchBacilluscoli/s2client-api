#include <sc2api/test.h>
#include <fstream>
#include <future>
#include <string>
#include <iostream>
#include <chrono>
#include <vector>
#include <sc2utils/sc2_manage_process.h>
#include <sys/wait.h>
#include <numeric>

#define CORS_COUNT 200

namespace sc2
{
    void ANBot::OnGameEnd()
    {
    }

    void ANBot::OnUnitIdle(const Unit *u)
    {
        Units enemies = Observation()->GetUnits(Unit::Alliance::Enemy);
        Actions()->UnitCommand(u, ABILITY_ID::ATTACK, enemies.front());
    }

    void Test::Update(Coordinator &cor, int frames, std::ostream &os)
    {
        static int current_loop = 0;
        for (int i = 0; i < frames; ++i)
        {
            try
            {
                cor.Update();
            }
            catch (const std::exception &e)
            {
                os << "update:\t" << i << '\t' << cor.GetPortStart() << std::endl;
            }
        }
        }

    void Test::CSetAndStart(Coordinator &cor, int port, int argc, char *argv[], Agent *bot1, Agent *bot2, int time_out_ms, std::ostream &os)
    {
        cor.LoadSettings(argc, argv);
        // cor.SetTimeoutMS(time_out_ms);
        cor.SetParticipants({CreateParticipant(Terran, bot1),
                             CreateComputer(Terran)});
        PortChecker pc;
        for (int i = 0; i < 7; ++i) //得到连续7个port
        {
            if (i != 2 && !pc.Check(port + i))
            {
                port = port + i + 1;
                i = 0;
            }
        }
        SleepFor(500);
        cor.SetPortStart(port + 1);
        cor.SetMapPath("PCANP_EnemyZealotModVSMarines.SC2Map");
        cor.SetStepSize(1);
        try
        {
            cor.LaunchStarcraft();
        }
        catch (const std::exception &e)
        {
            os << "launch都不成功:\t" << port << std::endl;
        }
        try
        {
            if (cor.StartGame() == false)
            {
                os << "启动不成功居然就返回了: " << port << std::endl;

                // 有任何问题，先杀掉原始程序
                std::vector<ProcessInfo> infos = cor.GetProcessInfo();
                for (int i = 0; i < 2; ++i)
                {

                    if (IsProcessRunning(infos[0].process_id))
                    {
                        TerminateProcess(infos[0].process_id);
                    }
                    if (infos.size() > 1 && IsProcessRunning(infos[1].process_id))
                    {
                        TerminateProcess(infos[1].process_id);
                    }
                    SleepFor(500);
                }
                int status;
                ::waitpid(infos[0].process_id, &status, WUNTRACED | WCONTINUED); // kill the zombie ps
                if (infos.size() > 1)
                {
                    ::waitpid(infos[1].process_id, &status, WUNTRACED | WCONTINUED);
                }
                SleepFor(1000);

                cor.ClearOldProcessInfo();
                bot1->Reset();
                bot2->Reset();
                PortChecker pc;
                //todo 得到7个ports
                for (int i = 0; i < 7; ++i) //得到连续7个port
                {
                    if (i != 2 && !pc.Check(port + i))
                    {
                        port = port + i + 1;
                        i = 0;
                    }
                }
                cor.SetPortStart(port + 1);

                //! 恐怕需要先杀掉原先的进程才行...可是StartGame不是杀过了吗？
                cor.LaunchStarcraft();
                cor.StartGame();
            }
        }
        catch (const std::exception &e)
        {
            os << "启动不成功居然就返回了（异常）: " << port << std::endl;
            std::vector<ProcessInfo> infos = cor.GetProcessInfo(); // 杀进程
            for (int i = 0; i < 2; ++i)
            {

                if (IsProcessRunning(infos[0].process_id))
                {
                    TerminateProcess(infos[0].process_id);
                }
                if (infos.size() > 1 && IsProcessRunning(infos[1].process_id))
                {
                    TerminateProcess(infos[1].process_id);
                }
                SleepFor(500);
            }
            int status; // 等待的方式杀掉僵尸进程
            ::waitpid(infos[0].process_id, &status, WUNTRACED | WCONTINUED);
            if (infos.size() > 1)
            {
                ::waitpid(infos[1].process_id, &status, WUNTRACED | WCONTINUED);
            }
            SleepFor(1000);
            cor.ClearOldProcessInfo();
            bot1->Reset();
            bot2->Reset();
            PortChecker pc;
            for (int i = 0; i < 7; ++i) //得到连续7个port
            {
                if (i != 2 && !pc.Check(port + i))
                {
                    port = port + i + 1;
                    i = 0;
                }
            }
            cor.SetPortStart(port + 1);
            cor.LaunchStarcraft();
            if (!cor.StartGame())
            {
                os << "还是没成功：\t" << port << std::endl;
            }
        }
    }

    void Test::SetAndStartCors(int argc, char *argv[])
    {
        std::fstream file("./time.txt", std::ios::app);
        int update_frames = 50;
        int port_start = 61000;
        int time_out_ms = 10000;
        int batch_count = 50;
        bool one_by_one = false;

        std::vector<Coordinator> cors(CORS_COUNT);
        ANBot bots1[CORS_COUNT];
        ANBot bots2[CORS_COUNT];
        std::vector<std::future<void>> cors_futures(CORS_COUNT);

        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

        int count = CORS_COUNT;

        if (one_by_one)
        {
            // while (1)
            // {
            for (int i = 0; i < count; ++i)
            {
                CSetAndStart(cors[i], port_start + i * 20, argc, argv, &(bots1[i]), &(bots2[i]), time_out_ms, file);
                file << i << '\t' << std::flush;
            }
            // }
            file << count << std::endl;
        }
        else
        {
            for (int i = 0; i < count; ++i)
            {
                cors_futures[i] = std::async(std::launch::async, &Test::CSetAndStart, std::ref(cors[i]), port_start + 20 * i, argc, argv, &(bots1[i]), &(bots2[i]), time_out_ms, std::ref(file));
                if (i != 0 && (i + 1) % batch_count == 0)
                {
                    for (int j = i - batch_count + 1; j <= i; ++j) // wait on every several launches
                    {
                        cors_futures[j].wait();
                    }
                }
            }
        }

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::chrono::steady_clock::duration time_used = end - start;
        file << "launch (" << CORS_COUNT << "," << batch_count << ", ms):\t" << (std::chrono::duration_cast<std::chrono::milliseconds>(time_used)).count() << std::endl;
        for (size_t i = 0; i < 5; ++i)
        {
            std::vector<std::future<void>> cors_update_futures(CORS_COUNT);
            for (int i = 0; i < count; ++i)
            {
                cors_update_futures[i] = std::async(&Test::Update, std::ref(cors[i]), update_frames, std::ref(file));
            }
            for (int i = 0; i < count; ++i)
            {
                cors_update_futures[i].wait();
            }

            for (size_t i = 0; i < CORS_COUNT; ++i)
            {
                auto us = bots1[i].Observation()->GetUnits();
                float sum = std::accumulate(us.begin(), us.end(), 0.f, [](float first, const Unit *u) -> float {
                    return first + u->health;
                });
                file << sum << ':';
                file << bots1[i].Observation()->GetGameLoop() << '\t';
            }
            file << std::endl;
        } // namespace sc2

        std::chrono::steady_clock::time_point update_end = std::chrono::steady_clock::now();
        file << "update(" << update_frames
             << ", ms):\t" << (std::chrono::duration_cast<std::chrono::milliseconds>(update_end - end)).count() << std::endl;
        file.close();
        return;
    } // namespace sc2

} // namespace sc2
