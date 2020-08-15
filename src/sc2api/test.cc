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

    void Test::Update(Coordinator &cor, int frames)
    {
        for (int i = 0; i < frames; ++i)
        {
            cor.Update();
        }
    }

    void Test::CSetAndStart(Coordinator &cor, int port, int argc, char *argv[], Agent *bot1, Agent *bot2, int time_out_ms)
    {
        cor.LoadSettings(argc, argv);
        cor.SetTimeoutMS(time_out_ms);
        cor.SetParticipants({CreateParticipant(Terran, bot1), CreateParticipant(Terran, bot2)});
        //todo check here

        cor.SetPortStart(port);
        cor.SetMapPath("PCANP_EnemyZealotModVSMarines.SC2Map");
        cor.SetStepSize(1);
        try
        {
            cor.LaunchStarcraft();
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
            std::vector<ProcessInfo> infos = cor.GetProcessInfo();
            for (int i = 0; i < 5; ++i)
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
            ::waitpid(infos[0].process_id, &status, WUNTRACED | WCONTINUED); // 杀掉僵尸进程
            if (infos.size() > 1)
            {
                ::waitpid(infos[1].process_id, &status, WUNTRACED | WCONTINUED);
            }
            SleepFor(1000);
            cor.ClearOldProcessInfo();

            bool checked = false;
            thread_local PortChecker pc;
            //todo 得到两个可用端口
            while (!(pc.Check(port) && pc.Check(port + 1)))
            {
                if (!pc.Check(port + 1))
                {
                    port += 2;
                }
                else
                {
                    port += 1;
                }
            }
            cor.SetPortStart(port + 1); // 因为启动的时候第一个port会-1

            cor.LaunchStarcraft();
        }
        while (cor.StartGame() == false)
        {
            std::cout << "启动不成功居然就返回了" << std::endl;
            cor.ClearOldProcessInfo();
            bot1->Reset();
            bot2->Reset();
            thread_local PortChecker pc;
            std::vector<int> ports(5);
            //todo 得到5个ports

            int ports_num = 0;
            int i = 2; // 从port后第2个开始
            while (ports_num < 5)
            {
                if (pc.Check(port + i))
                {
                    ports[ports_num] = port + i;
                    ++ports_num;
                }
                ++i;
            }
            //! 恐怕需要先杀掉原先的进程才行...可是StartGame不是杀过了吗？

            cor.LaunchStarcraft();
            cor.SetupPorts(2, ports);
        }
    }

    void Test::SetAndStartCors(int argc, char *argv[])
    {
        std::fstream file("./time.txt", std::ios::app);
        int update_frames = 100;
        int port_start = 60000;
        int time_out_ms = 10000;
        int batch_count = 20;
        bool one_by_one = false;

        std::vector<Coordinator> cors(CORS_COUNT);
        ANBot bots1[CORS_COUNT];
        ANBot bots2[CORS_COUNT];
        std::vector<std::future<void>> cors_futures(CORS_COUNT);

        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

        int count = CORS_COUNT;

        if (one_by_one)
        {
            for (int i = 0; i < count; ++i)
            {
                CSetAndStart(cors[i], port_start + i * 20, argc, argv, &(bots1[i]), &(bots2[i]));
            }
        }
        else
        {
            for (int i = 0; i < count; ++i)
            {
                cors_futures[i] = std::async(std::launch::async, &Test::CSetAndStart, std::ref(cors[i]), port_start + 10 * i, argc, argv, &(bots1[i]), &(bots2[i]), time_out_ms);
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

        std::vector<std::future<void>> cors_update_futures(CORS_COUNT);
        for (int i = 0; i < count; ++i)
        {
            cors_update_futures[i] = std::async(&Test::Update, std::ref(cors[i]), update_frames);
        }
        for (int i = 0; i < count; ++i)
        {
            cors_update_futures[i].wait();
        }

        //todo output the result
        for (size_t i = 0; i < CORS_COUNT; ++i)
        {
            auto us = bots1[i].Observation()->GetUnits();
            float sum = std::accumulate(us.begin(), us.end(), 0.f, [](float first, const Unit *u) -> float {
                return first + u->health;
            });
            std::cout << sum << ':';
            std::cout << bots1[i].Observation()->GetGameLoop() << '\t';
        }
        std::cout << std::endl;

        std::chrono::steady_clock::time_point update_end = std::chrono::steady_clock::now();
        file << "update(" << update_frames
             << ", ms):\t" << (std::chrono::duration_cast<std::chrono::milliseconds>(update_end - end)).count() << std::endl;
        return;
    }

} // namespace sc2
