#include "global_defines.h"

#include <sc2api/sc2_api.h>
#include <string>
#include <iostream>
#include <chrono>
#include <sc2api/test.h> // added by myself
#include <sc2utils/sc2_manage_process.h>
#include <civetweb.h>
#include <sc2api/sc2_connection.h>

#include "./rolling_bot/rolling_bot/rolling_bot.h"

#include <sys/wait.h>

using namespace sc2;

//! for test use
class Bot : public Agent
{
private:
    Simulator sim = Simulator();

public:
    Bot(const std::string &ps_path, const std::string &map)
    {
        sim.SetProcessPath(ps_path);
        sim.SetMapPath(Simulator::GenerateSimMapPath(map));
    }
    virtual void OnGameStart() final
    {
        Debug()->DebugCreateUnit(UNIT_TYPEID::TERRAN_AUTOTURRET, {1, 1});
        Debug()->DebugCreateUnit(UNIT_TYPEID::TERRAN_AUTOTURRET, {80, 80});
        Debug()->DebugCreateUnit(UNIT_TYPEID::TERRAN_AUTOTURRET, {80, 1});
        Debug()->DebugCreateUnit(UNIT_TYPEID::TERRAN_AUTOTURRET, {80, 80});
        Debug()->SendDebug();
    }

    virtual void OnUnitIdle(const Unit *unit) final
    {
    }

    virtual void OnUnitDestroyed(const Unit *unit) override
    {
    }

    virtual void OnStep() final
    {
        Units units = Observation()->GetUnits();
        for (size_t i = 0; i < units.size(); i++)
        {
            std::cout << units[i]->pos.x << ", " << units[i]->pos.y << std::endl;
        }
    }
};

// class ANBot : public Agent
// {
//     void OnGameEnd() final
//     {
//         // AgentControl()->Restart();
//     }

//     void OnUnitIdle(const Unit *u) final
//     {
//         Units enemies = Observation()->GetUnits(Unit::Alliance::Enemy);
//         Actions()->UnitCommand(u, ABILITY_ID::ATTACK, enemies.front());
//     }
// };

void SetAndStart(Simulator &sim, int port, int argc, char *argv[])
{
    sim.LoadSettings(argc, argv);
    sim.SetControlledPlayerNum(2);
    sim.SetPortStart(port);
    sim.SetMapPath("PCANP_EnemyZealotModVSMarines.SC2Map");
    sim.SetStepSize(1);
    sim.LaunchStarcraft();
    sim.StartGame();
}

void CSet(Coordinator &cor, int port, int argc, char *argv[], Agent *bot1, Agent *bot2)
{
    cor.LoadSettings(argc, argv);
    cor.SetParticipants({CreateParticipant(Terran, bot1)});
    cor.SetPortStart(port);
    cor.SetMapPath("PCANP_EnemyZealotModVSMarines.SC2Map");
    cor.SetStepSize(1);
}

void pringA()
{
    std::cout << "a" << std::endl;
}

int main(int argc, char *argv[])
{

    std::vector<Simulator> sims(100);
    std::vector<Coordinator> cors(100);
    Coordinator cor;
    StartCivetweb();
    {
        PortChecker pc;
        pc.GetContinuousPortsFromPort(61000, 30);
    }
    if (0)
    {
        while (1)
        {
            PortChecker pc;
            uint16_t new_port_num = pc.GetContinuousPortsFromPort(40918, 7);
            std::cout << new_port_num << std::endl;
            SleepFor(500);
        }
        return 0;
    }

    if (0)
    {
        Test::SetAndStartCors(argc, argv);

        std::vector<std::future<void>> sims_futures(100);
        for (int i = 0; i < 50; ++i)
        {
            sims_futures[i] = std::async(std::launch::async, SetAndStart, std::ref(sims[i]), 4000 + 10 * i, argc, argv);
        }
        for (int i = 0; i < 50; ++i)
        {
            sims_futures[i].wait();
        }
        std::string map_path = sims.front().GetMapPath();

        for (int i = 0; i < 50; ++i)
        {
            sims_futures[i] = std::async(&Test::Update, std::ref(sims[i]), 100, std::ref(std::cout));
        }

        std::vector<std::future<bool>> sims_futures2(100);
        for (size_t i = 0; i < 10; i++)
        {
            for (int j = 0; j < 10; ++j)
            {
                sims_futures2[i * 10 + j] = std::async(&Coordinator::StartGame, sims[i * 10 + j], map_path);
            }
            for (int j = 0; j < 10; ++j)
            {
                sims_futures2[i * 10 + j].wait();
            }
            // sims[i].StartGame();
        }
        for (int i = 0; i < 100; ++i)
        {
            sims_futures2[i].wait();
        }

        mg_exit_library();
        return 0;
    }

    while (true)
    {
        try
        {
            for (size_t i = 0; i < argc; i++)
            {
                std::cout << argv[i] << std::endl;
            }
#ifdef USE_SYSTEM_COMMAND                                                                // without automatically kill the sc2 processs, you need to kill them manually
            std::vector<std::string> process_names_to_be_killed({"SC2_x64", "gnuplot"}); // Before evething starts, kill all the superfluous processes started before.
            int kill_sz = process_names_to_be_killed.size();
            for (size_t i = 0; i < kill_sz; i++)
            {
                KillProcess(process_names_to_be_killed[i]);
            }
#endif //USE_SYSTEM_COMMAND

            // Some settings
            bool is_debug = true;
            std::string net_address = "127.0.0.1";

            int port_start = 61000;
            int main_process_port = 3900; //3989 is the default port of xrdp
            bool real_time = false;       // but if the graphics is on, the main game will be showed in real-time mode
            bool multi_threaded = false;
            uint frames = 60;       // use this to control the Update times per second
            int objective_size = 3; // 3 is the traditional setting
            bool use_enemy_pop = true;
            bool is_enemy_pop_evo = false;
            int population_size = 20;
            int max_generations = 40;
            int max_no_improve_generation = 100;
            int ga_muatation_rate = 0.5;
            int command_length = 50;
            int sim_length = 75;
            int interval_size = 50;
            int evaluation_multiplier = 1;
            PLAY_STYLE play_style = PLAY_STYLE::NORMAL;
            bool use_fix = false; // if the map is dynamic do not use the static fix -> will be aborted soon
            unsigned fix_by_data_interval = 6;

            bool use_fix_by_data = true;
            bool use_priori = false;
            bool use_assembled = true;

            bool use_output_file = false;
            bool set_output_conver = false;
            bool use_output_sim_record = false;
            bool use_output_v_and_o = false;
            bool only_start = false;

            std::string point_of_expriment = std::string(use_priori ? "priori " : "") + (use_fix ? "fix " : "") + (use_fix_by_data ? "data_fix " : "") + (use_assembled ? "assemble " : "");
            int game_round = 10;
            std::vector<std::string> record_remark_vec = {
                point_of_expriment + ", ",
                var2str(is_enemy_pop_evo) + ":",
                std::to_string(is_enemy_pop_evo) + ", ",
                var2str(play_style) + ": ",
                g_play_style_names[static_cast<int>(play_style)] + ", ",
                var2str(population_size) + ": ",
                std::to_string(population_size) + ", ",
                var2str(max_generations) + ": ",
                std::to_string(max_generations) + ", ",
                var2str(command_length) + ": ",
                std::to_string(command_length) + ", ",
                var2str(sim_length) + ": ",
                std::to_string(sim_length) + ", ",
                var2str(interval_size) + ": ",
                std::to_string(interval_size) + ", ",
                var2str(evaluation_multiplier) + ": ",
                std::to_string(evaluation_multiplier) + ", ",
                var2str(objective_size) + ": ",
                std::to_string(objective_size) + ", "};
            std::string record_remark = std::accumulate(record_remark_vec.begin(), record_remark_vec.end(), std::string());

#ifdef USE_GRAPHICS
            DebugRenderer renderer;
#endif //USE_GRAPHICS

            Coordinator coordinator;
            coordinator.LoadSettings(argc, argv);
            std::string starcraft_path = coordinator.GetExePath();
            coordinator.SetProcessPath(starcraft_path);
            std::string map_path = coordinator.GetMapPath(); // you can set your own map_path here

            //! Bots here
            Bot bot(coordinator.GetExePath(), map_path);
            RollingBot rolling_bot(net_address, port_start, starcraft_path, map_path, max_generations, population_size, use_enemy_pop, objective_size);

            PortChecker pc;
            main_process_port = pc.GetContinuousPortsFromPort(main_process_port, 7);
            std::random_device rd;
            auto random_seed = rd();

            rolling_bot.Algorithm().ConvergenceTermination()->SetMaxNoImproveGeneration(max_no_improve_generation);
            rolling_bot.Algorithm().SetDebug(is_debug);
            rolling_bot.Algorithm().SetAttackPossibility(1.f);
            rolling_bot.Algorithm().SetSimLength(sim_length);
            rolling_bot.Algorithm().SetCommandLength(command_length);
            rolling_bot.Algorithm().SetEvaluationTimeMultiplier(evaluation_multiplier);
            rolling_bot.Algorithm().SetRandomEngineSeed(random_seed);
            rolling_bot.Algorithm().SetUseFix(use_fix);
            rolling_bot.Algorithm().SetUseFixByData(use_fix_by_data);
            rolling_bot.Algorithm().SetFixByDataInterval(fix_by_data_interval);
            rolling_bot.Algorithm().SetUsePriori(use_priori);
            rolling_bot.Algorithm().SetUseAssemble(use_assembled);
            rolling_bot.Algorithm().TerminationCondition(TERMINATION_CONDITION::CONVERGENCE);
            static_cast<RollingDE &>(rolling_bot.Algorithm()).SetEnemyPopEvo(is_enemy_pop_evo); // ugly, but I the feeling of using static_cast
            rolling_bot.SetIntervalLength(interval_size);
            rolling_bot.SetStyle(play_style);
            rolling_bot.SetRemark(record_remark);
            rolling_bot.SetOnlyStart(only_start);
            rolling_bot.Algorithm().SetUseOutputFile(use_output_file);
            if (only_start) // 只有仅记录start的情况下数据才有用
            {
                rolling_bot.Algorithm().SetOutputVAndO(use_output_v_and_o);
                rolling_bot.SetOutputConver(set_output_conver);
                rolling_bot.SetOutputSimRecord(use_output_sim_record);
            }

            // std::cout << rolling_bot.Algorithm().GetMaxGeneration() << std::endl;

            //! participants settings here
            coordinator.SetParticipants({CreateParticipant(Race::Terran, &rolling_bot),
                                         CreateComputer(Race::Terran)});
            coordinator.SetPortStart(main_process_port + 1);
            coordinator.SetRealtime(real_time);
            coordinator.SetMultithreaded(multi_threaded);

            const ObservationInterface *ob = coordinator.GetObservations().front();

            try
            {
                coordinator.LaunchStarcraft();
            }
            catch (const std::exception &e)
            {
                PortChecker pc;
                std::cerr << e.what() << '\n';
                coordinator.LeaveGame();
                std::vector<ProcessInfo> infos = coordinator.GetProcessInfo();
                for (int i = 0; i < 3; ++i)
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
                coordinator.ClearOldProcessInfo();
                main_process_port = pc.GetContinuousPortsFromPort(coordinator.GetPortStart(), 7);
                coordinator.SetPortStart(main_process_port + 1);
                coordinator.LaunchStarcraft();
                SleepFor(1000);
            }
            // while (!coordinator.StartGame())
            // {
            //     PortChecker pc;
            //     coordinator.LeaveGame();
            //     std::vector<ProcessInfo> infos = coordinator.GetProcessInfo();
            //     for (int i = 0; i < 3; ++i)
            //     {

            //         if (IsProcessRunning(infos[0].process_id))
            //         {
            //             TerminateProcess(infos[0].process_id);
            //         }
            //         if (infos.size() > 1 && IsProcessRunning(infos[1].process_id))
            //         {
            //             TerminateProcess(infos[1].process_id);
            //         }
            //         SleepFor(500);
            //     }
            //     int status;
            //     ::waitpid(infos[0].process_id, &status, WUNTRACED | WCONTINUED); // kill the zombie ps
            //     if (infos.size() > 1)
            //     {
            //         ::waitpid(infos[1].process_id, &status, WUNTRACED | WCONTINUED);
            //     }
            //     SleepFor(1000);
            //     coordinator.ClearOldProcessInfo();
            //     uint16_t new_port_start = pc.GetContinuousPortsFromPort(coordinator.GetPortStart(), 7);
            //     coordinator.SetPortStart(new_port_start + 1);
            //     coordinator.LaunchStarcraft();
            //     SleepFor(1000);
            // }
            coordinator.StartGame();
            // SleepFor(10000);
            // A fixed time update mechanism
            auto start = std::chrono::steady_clock::now();
            auto end = std::chrono::steady_clock::now();
            while (
#ifdef REAL_TIME_UPDATE
                start = std::chrono::steady_clock::now(),
#endif // REAL_TIME_UPDATE
#ifdef USE_GRAPHICS
                renderer.ClearRenderer(),
                renderer.DrawObservation(ob), // display the game, since StartGame() runs for 1 starting frame, it can not display it by renderer here.
                renderer.Present(),
#endif //USE_GRAPHICS
                coordinator.Update())
            {
#ifdef REAL_TIME_UPDATE
                end = std::chrono::steady_clock::now();
                auto interval = end - start;
                std::this_thread::sleep_for(std::chrono::milliseconds(1000 / frames) - interval);
#endif // REAL_TIME_UPDATE
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
            SleepFor(6000);
            continue;
        }
    }
    mg_exit_library();
    return 0;
}