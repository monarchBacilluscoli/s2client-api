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
    //! test code
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
    // RollingBot rolling_bot(net_address, port_start, starcraft_path, map_path);
    // rolling_bot.SetMaxGeneration(10);

    // //! for coordinator test
    // Coordinator test_coordinator;
    // test_coordinator.SetProcessPath(starcraft_path);
    // test_coordinator.SetMapPath(map_path);
    // test_coordinator.SetPortStart(4000);
    // test_coordinator.SetParticipants(
    //     {CreateParticipant(Race::Terran, &bot), CreateComputer(Race::Protoss)});
    // test_coordinator.LaunchStarcraft();
    // // test_coordinator.Connect(3999);
    // test_coordinator.StartGame();

    //! participants settings here
    coordinator.SetParticipants({CreateParticipant(Race::Terran, &bot),
                                 CreateComputer(Race::Zerg)});
    coordinator.SetPortStart(main_process_port);
    coordinator.SetRealtime(real_time);
    coordinator.SetMultithreaded(multi_threaded);

    const ObservationInterface* ob = coordinator.GetObservations().front();

    coordinator.LaunchStarcraft();
    coordinator.StartGame(map_path);

    // Another thread used for debuging
    // std::thread display_thread = std::thread([/*&rolling_bot,*/&is_debug, &ob]() {
    //     DebugRenderer sim_debug_renderer("Debug Observer");
    //     // std::vector<const ObservationInterface*> observations = rolling_bot.GetAllSimsObservations();
    //     //todo set a timer here
    //     while (is_debug) {
    //         sim_debug_renderer.DrawObservation(ob);
    //         // sim_debug_renderer.DrawObservations(observations);
    //     }
    // });
    // display_thread.detach();

    // A fixed time update mechanism
    auto start = std::chrono::steady_clock::now();
    auto end = std::chrono::steady_clock::now();
    while (start = std::chrono::steady_clock::now(),
           renderer.DrawObservation(ob),  // display the game, since StartGame() runs for 1 starting frame, it can not display it by renderer here.
           coordinator.Update()) {
        end = std::chrono::steady_clock::now();
        auto interval = end - start;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / frames) -
                                    interval);
        
    }
    // is_debug = false;

    return 0;
}