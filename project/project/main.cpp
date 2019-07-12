#include <sc2api/sc2_api.h>
#include <debug_renderer/debug_renderer.h>
#include <string>
#include <iostream>
#include <chrono>
#include <thread>

//! test include files
#include "../rolling_bot/simulator/state.h"
#include "../rolling_bot/algorithm/ga.h"
#include "../rolling_bot/algorithm/real_ga.h"
#include "../rolling_bot/rolling_bot/rolling_bot.h"

using namespace sc2;

//! for test use
class Bot : public Agent {
public:
    Bot(){
        m_renderer.SetIsDisplay(false);
    }
    virtual void OnGameStart() final {
        std::cout << "Hello, World!" << std::endl;
    }

    virtual void OnUnitIdle(const Unit* unit) final {
        Units enemies = Observation()->GetUnits(Unit::Alliance::Enemy);
        Actions()->UnitCommand(unit,ABILITY_ID::ATTACK,GetRandomEntry(enemies),true);
    }

    virtual void OnStep() final {
        std::cout << Observation()->GetGameLoop() << std::endl;
        m_renderer.DrawObservation(Observation());
    }

private:
    DebugRenderer m_renderer;
};

int main(int argc, char* argv[]) {
    //! test code (Can be folded)
    {
        //! for real ga
        // {
        //     RealGA real_ga;
        //     RealGA::SetExampleDimensions(3);
        //     auto a = &RealGA::example_evaluator;
        //     real_ga.SetEvaluator(&RealGA::example_evaluator);
        //     real_ga.SetBoundry(RealGA::example_lower_boundry, RealGA::example_upper_boundry);
        //     Solution<float> final = real_ga.Run().front();
        // }
        //! A DebugRenderers test
        // {
        //     int count = 3;
        //     std::vector<Bot> bots(count);
        //     std::vector<Coordinator> coordinators(count);
        //     std::vector<const ObservationInterface*> observations(count);
        //     std::vector<std::thread> start_threads(count);
        //     for (size_t i = 0; i < count; i++) {
        //         start_threads[i] = std::thread([&,i] {
        //             coordinators[i].LoadSettings(argc, argv);
        //             // coordinators[i].SetProcessPath("/home/liuyongfeng/StarCraftII/Versions/Base70154/SC2_x64");
        //             // std::cout << coordinators[i].GetExePath() << std::endl;
        //             coordinators[i].SetPortStart(5000 + i * 2);
        //             coordinators[i].SetParticipants({CreateParticipant(Race::Terran, &bots[i]),
        //                                              CreateComputer(Race::Zerg)});
        //             coordinators[i].LaunchStarcraft();
        //             coordinators[i].StartGame("testBattle_distant_vs_melee_debug.SC2Map");
        //             observations[i] = coordinators[i].GetObservations().front();
        //         });
        //     }
        //     for(auto& t: start_threads){
        //         t.join();
        //     }
        //     DebugRenderers debug_renderers(count);
        //     auto start = std::chrono::steady_clock::now();
        //     auto end = std::chrono::steady_clock::now();
        //     while (true) {
        //         start = std::chrono::steady_clock::now();
        //         for (size_t i = 0; i < count; i++) {
        //             coordinators[i].Update();
        //             debug_renderers[i].DrawObservation(observations[i]);
        //             // debug_renderers[i].DrawRedRect();
        //         }
        //         end = std::chrono::steady_clock::now();
        //         auto interval = end - start;
        //         std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 60) -
        //                                     interval);
        //     }
        // }
    }
    {
        // //! Test for the simulator
        // //
        // Simulator sim;
        // DebugRenderer debug_renderer;
        // sim.SetProcessPath("/home/liuyongfeng/StarCraftII/Versions/Base70154/SC2_x64");
        // sim.SetPortStart(3000);
        // sim.SetMapPath("testBattle_distant_vs_melee_debug.SC2Map");
        // sim.LaunchStarcraft();
        // sim.StartGame();

        
        // //todo randomly creates units in a range
        // for (size_t i = 0; true; i++) {
        //     Units us = sim.Observation()->GetUnits();
        //     std::for_each(us.begin(), us.end(), [&](const Unit* u) { sim.Debug()->DebugKillUnit(u); });
        //     sim.Debug()->SendDebug();
        //     for (size_t i = 0; i < 20; i++) {
        //         std::cout << "unit number: " << sim.Observation()->GetUnits().size() << std::endl;
        //         sim.Update();
        //         std::cout << "unit number: " << sim.Observation()->GetUnits().size() << std::endl;
        //         debug_renderer.DrawObservation(sim.Observation());
        //     }
        //     int unit_count = 20;
        //     // for (size_t i = 0; i < unit_count; i++) {
        //     //     float random = GetRandomFraction();
        //     //     UnitTypeID ut = random > 0.5 ? UNIT_TYPEID::TERRAN_MARINE : UNIT_TYPEID::PROTOSS_ZEALOT;
        //     //     uint32_t player_id = random > 0.5 ? 1 : 2;
        //     //     sim.Debug()->DebugCreateUnit(ut, FindRandomLocation({16, 16}, {17, 17}), player_id);
        //     // }
        //     // sim.Debug()->DebugCreateUnit(UNIT_TYPEID::TERRAN_MARINE, FindRandomLocation({16, 16}, {17, 17}), 1, unit_count);
        //     for (size_t i = 0; i < unit_count; i++)
        //     {
        //         float random = GetRandomFraction();
        //         UnitTypeID ut = random > 0.5 ? UNIT_TYPEID::TERRAN_MARINE : UNIT_TYPEID::PROTOSS_ZEALOT;
        //         uint32_t player_id = random > 0.5 ? 1 : 2;
        //         sim.Debug()->DebugCreateUnit(ut, FindRandomLocation({16, 16}, {17, 17}), player_id);
        //     }

        //     sim.Debug()->SendDebug();
        //     for (size_t i = 0; i < 2; i++) {
        //         std::cout << "unit number: " << sim.Observation()->GetUnits().size() << std::endl;
        //         sim.Update();
        //         std::cout << "unit number: " << sim.Observation()->GetUnits().size() << std::endl;
        //         debug_renderer.DrawObservation(sim.Observation());
        //     }
        //     std::cout << "the result: " << sim.Observation()->GetUnits().size() << std::endl;
        //     {
        //         // std::cout << "1: ";
        //         // sim.CopyAndSetState(sim.Observation(), &debug_renderer);
        //         // std::cout << "2: ";
        //         // sim.CopyAndSetState(sim.Observation(), &debug_renderer);
        //         // std::cout << "3: ";
        //         // sim.CopyAndSetState(sim.Observation(), &debug_renderer);
        //     }

        //     std::cout << std::endl;
        // }
    }

    // Before evething starts, kill all the StarCraftII instances started before.
    std::string process_name_to_be_killed = "SC2_x64";
    std::string command = "ps -aux| grep " + process_name_to_be_killed + " | grep -v grep |awk '{print $2}' | xargs kill";
    system(command.c_str());

    // Some settings
    bool is_debug = true;
    std::string net_address = "127.0.0.1";
    std::string map_path = "testBattle_distant_vs_melee_debug.SC2Map";
    std::string starcraft_path = "/home/liuyongfeng/StarCraftII/Versions/Base70154/SC2_x64";    
    int port_start = 4000;
    int main_process_port = 5379;
    bool real_time = false;
    bool multi_threaded = false;
    // use this to control the cauculation times per second
    uint frames = 60;

    DebugRenderer renderer;

    Coordinator coordinator;
    coordinator.LoadSettings(argc, argv);

    //! Bots here
    Bot bot;
    RollingBot rolling_bot(net_address, port_start, starcraft_path, map_path);
    rolling_bot.SetDebugOn(true);

    rolling_bot.SetMaxGeneration(10);

    //! participants settings here
    coordinator.SetParticipants({CreateParticipant(Race::Terran, &rolling_bot),
                                 CreateComputer(Race::Zerg)});
    coordinator.SetPortStart(main_process_port);
    coordinator.SetRealtime(real_time);
    coordinator.SetMultithreaded(multi_threaded);

    const ObservationInterface* ob = coordinator.GetObservations().front();

    coordinator.LaunchStarcraft();
    coordinator.StartGame(map_path);

    // A fixed time update mechanism
    auto start = std::chrono::steady_clock::now();
    auto end = std::chrono::steady_clock::now();
    while (start = std::chrono::steady_clock::now(),
           renderer.ClearRenderer(),
           renderer.DrawObservation(ob),  // display the game, since StartGame() runs for 1 starting frame, it can not display it by renderer here.
           renderer.Present(),
           coordinator.Update()) {
        end = std::chrono::steady_clock::now();
        auto interval = end - start;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / frames) -
                                    interval);
        
    }
    // is_debug = false;

    return 0;
}