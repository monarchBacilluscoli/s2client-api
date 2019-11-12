#include <sc2api/sc2_api.h>
#include <string>
#include <iostream>
#include <chrono>

//! test include files
#include "../rolling_bot/rolling_bot/rolling_bot.h"

using namespace sc2;

//! for test use
class Bot : public Agent
{
public:
    Bot()
    {
        m_renderer.SetIsDisplay(false);
    }
    virtual void OnGameStart() final {}

    virtual void OnUnitIdle(const Unit *unit) final
    {
        Units enemies = Observation()->GetUnits(Unit::Alliance::Enemy);
        Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, GetRandomEntry(enemies));
    }

    virtual void OnUnitDestroyed(const Unit *unit) override
    {
        std::cout << "Player " << unit->owner << "'s unit "
                  << unit->unit_type << " died" << std::endl;
    }

    virtual void OnStep() final
    {
        std::cout << Observation()->GetUnits().size() << "\t";
    }

private:
    DebugRenderer m_renderer;
    RawActions m_stored_actions;
};

int main(int argc, char *argv[])
{
    for (size_t i = 0; i < argc; i++)
    {
        std::cout << argv[i] << std::endl;
    }

    // Before evething starts, kill all the superfluous processes started before.
    std::vector<std::string> process_names_to_be_killed({"SC2_x64", "gnuplot"});
    int kill_sz = process_names_to_be_killed.size();
    for (size_t i = 0; i < kill_sz; i++)
    {
        std::string kill_command = "ps -aux| grep " + process_names_to_be_killed[i] + " | grep -v grep |awk '{print $2}' | xargs kill";
        system(kill_command.c_str());
    }

    // Some settings
    bool is_debug = false;
    std::string net_address = "127.0.0.1";
    // std::string map_path = "testBattle_distant_vs_melee_debug.SC2Map";
    std::string map_path = "EnemyTower.SC2Map";
    // std::string map_path = "EnemyTowerVSMarineMarauder.SC2Map";
    // std::string map_path = "EnemyTowerVSThors.SC2Map";
    // std::string map_path = "EnemyTowerVSThorsOptimizationTest.SC2Map";
    // std::string map_path = "EnemyTowerVSThor.SC2Map";
    // std::string map_path = "EnemyTowerVSThorMarine.SC2Map";

    std::string starcraft_path = "/home/liuyongfeng/StarCraftII/Versions/Base70154/SC2_x64";
    int port_start = 4000;
    int main_process_port = 6379;
    bool real_time = false;
    bool multi_threaded = false;
    // use this to control the cauculation times per second
    uint frames = 60;
    int population_size = 50;
    int max_generations = 200;
    int ga_muatation_rate = 0.5;
    int command_length = 12;
    int sim_length = 400;
    int interval_size = 400;
    int evaluation_multiplier = 1;

    DebugRenderer renderer;

    Coordinator coordinator;
    coordinator.SetProcessPath(starcraft_path);
    coordinator.LoadSettings(argc, argv);

    //! Bots here
    Bot bot;
    RollingBot rolling_bot(net_address, port_start, starcraft_path, map_path, max_generations, 50);

    rolling_bot.Algorithm().SetDebug(is_debug);
    rolling_bot.Algorithm().SetSimLength(sim_length);
    rolling_bot.Algorithm().SetCommandLength(command_length);
    rolling_bot.Algorithm().SetEvaluationTimeMultiplier(evaluation_multiplier);
    rolling_bot.SetIntervalLength(interval_size);
    rolling_bot.SetStyle(PLAY_STYLE::NORMAL);
    // dynamic_cast<RollingGA&>(rolling_bot.Algorithm()).SetMutationRate(ga_muatation_rate);

    // RollingBot2 rolling_bot(net_address, port_start, starcraft_path, map_path);

    //! participants settings here
    coordinator.SetParticipants({CreateParticipant(Race::Terran, &rolling_bot),
                                 CreateComputer(Race::Zerg)});
    coordinator.SetPortStart(main_process_port);
    coordinator.SetRealtime(real_time);
    coordinator.SetMultithreaded(multi_threaded);

    const ObservationInterface *ob = coordinator.GetObservations().front();

    coordinator.LaunchStarcraft();
    coordinator.StartGame(map_path);

    // A fixed time update mechanism
    auto start = std::chrono::steady_clock::now();
    auto end = std::chrono::steady_clock::now();
    while (start = std::chrono::steady_clock::now(),
           renderer.ClearRenderer(),
           renderer.DrawObservation(ob), // display the game, since StartGame() runs for 1 starting frame, it can not display it by renderer here.
           renderer.Present(),
           coordinator.Update())
    {
        end = std::chrono::steady_clock::now();
        auto interval = end - start;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / frames) - interval);
    }

    return 0;
}