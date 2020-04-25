#include "global_defines.h"

#include "./rolling_bot/rolling_bot/rolling_bot.h"
#include <sc2api/sc2_api.h>
#include <string>
#include <iostream>
#include <chrono>

using namespace sc2;

//! for test use
class Bot : public Agent
{
private:
    Simulator sim;

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

int main(int argc, char *argv[])
{
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

            // std::string starcraft_path;;
            int port_start = 4000;
            int main_process_port = 6379;
            bool real_time = false; // but if the graphics is on, the main game will be showed in real-time mode
            bool multi_threaded = false;
            // use this to control the cauculation times per second
            uint frames = 60;
            int population_size = 50;
            int max_generations = 100;
            int max_no_improve_generation = 100;
            int ga_muatation_rate = 0.5;
            int command_length = 50;
            int sim_length = 300;
            int interval_size = 200;
            int evaluation_multiplier = 1;
            PLAY_STYLE play_style = PLAY_STYLE::NORMAL;
            bool use_fix = false; // if the map is dynamic do not use the static fix
            unsigned fix_by_data_interval = 6;

            bool use_fix_by_data = true;
            bool use_priori = true;
            bool use_assembled = true;

            bool use_output_file = false;
            bool set_output_conver = false;
            bool use_output_sim_record = false;
            bool only_start = false;

            std::string point_of_expriment = std::string(use_priori ? "priori " : "") + (use_fix ? "fix " : "") + (use_fix_by_data ? "data_fix " : "") + (use_assembled ? "assemble " : "");
            int game_round = 10;
            std::vector<std::string> record_remark_vec = {
                point_of_expriment + ", ",
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
            };
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
            RollingBot rolling_bot(net_address, port_start, starcraft_path, map_path, max_generations, population_size);

            rolling_bot.Algorithm().ConvergenceTermination()->SetMaxNoImproveGeneration(max_no_improve_generation);
            rolling_bot.Algorithm().SetDebug(is_debug);
            rolling_bot.Algorithm().SetAttackPossibility(1.f);
            rolling_bot.Algorithm().SetSimLength(sim_length);
            rolling_bot.Algorithm().SetCommandLength(command_length);
            rolling_bot.Algorithm().SetEvaluationTimeMultiplier(evaluation_multiplier);
            rolling_bot.Algorithm().SetRandomEngineSeed(1);
            rolling_bot.Algorithm().SetUseFix(use_fix);
            rolling_bot.Algorithm().SetUseFixByData(use_fix_by_data);
            rolling_bot.Algorithm().SetFixByDataInterval(fix_by_data_interval);
            rolling_bot.Algorithm().SetUsePriori(use_priori);
            rolling_bot.Algorithm().SetUseAssemble(use_assembled);
            rolling_bot.Algorithm().TerminationCondition(TERMINATION_CONDITION::CONVERGENCE);
            rolling_bot.SetIntervalLength(interval_size);
            rolling_bot.SetStyle(play_style);
            rolling_bot.SetRemark(record_remark);
            rolling_bot.Algorithm().SetUseOutputFile(use_output_file);
            rolling_bot.SetOutputConver(set_output_conver);
            rolling_bot.SetOutputSimRecord(use_output_sim_record);
            rolling_bot.SetOnlyStart(only_start);

            // std::cout << rolling_bot.Algorithm().GetMaxGeneration() << std::endl;

            //! participants settings here
            coordinator.SetParticipants({CreateParticipant(Race::Terran, &rolling_bot),
                                         CreateComputer(Race::Terran)});
            coordinator.SetPortStart(main_process_port);
            coordinator.SetRealtime(real_time);
            coordinator.SetMultithreaded(multi_threaded);

            const ObservationInterface *ob = coordinator.GetObservations().front();

            coordinator.LaunchStarcraft();
            coordinator.StartGame();

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
            /* code */
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
            continue;
        }
    }
    return 0;
}