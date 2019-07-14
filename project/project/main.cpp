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
        //todo add attack then move action to each unit
        GameInfo game_info = Observation()->GetGameInfo();
        Units self_us = Observation()->GetUnits(Unit::Alliance::Self);
        Units enemy_us = Observation()->GetUnits(Unit::Alliance::Enemy);
        for (const Unit *u : self_us)
        {
            Actions()->UnitCommand(u, ABILITY_ID::ATTACK, GetRandomEntry(enemy_us), true);
        }
        //todo add move actions
        for (const Unit* u: self_us)
        {
            for (size_t i = 0; i < 3; i++)
            {
                Actions()->UnitCommand(u,ABILITY_ID::MOVE, FindRandomLocation(game_info), true);
            }
        }
        
    }

    virtual void OnUnitIdle(const Unit* unit) final {
        Units enemies = Observation()->GetUnits(Unit::Alliance::Enemy);
        // Actions()->UnitCommand(unit,ABILITY_ID::ATTACK,GetRandomEntry(enemies),true);
    }

    virtual void OnStep() final {
        std::cout << Observation()->GetGameLoop() << std::endl;
        m_renderer.DrawObservation(Observation());
    }

private:
    DebugRenderer m_renderer;
};

int main(int argc, char* argv[]) {
    // Before evething starts, kill all the StarCraftII instances started before.
    std::string process_name_to_be_killed = "SC2_x64";
    std::string command = "ps -aux| grep " + process_name_to_be_killed + " | grep -v grep |awk '{print $2}' | xargs kill";
    system(command.c_str());
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
        // Simulator sim;
        // DebugRenderer debug_renderer;
        // sim.SetProcessPath("/home/liuyongfeng/StarCraftII/Versions/Base70154/SC2_x64");
        // sim.SetPortStart(3000);
        // sim.SetMapPath("testBattle_distant_vs_melee_debug.SC2Map");
        // sim.LaunchStarcraft();
        // sim.StartGame();

        
        // // //todo randomly creates units in a range
        // for (size_t i = 0; true; i++) {
        //     Units us = sim.Observation()->GetUnits();
        //     std::for_each(us.begin(), us.end(), [&](const Unit* u) { sim.Debug()->DebugKillUnit(u); });
        //     sim.Debug()->SendDebug();
            
        //     // Create 20 units
        //     int unit_count = 20;
        //     for (size_t i = 0; i < unit_count; i++)
        //     {
        //         float random = GetRandomFraction();
        //         UnitTypeID ut = random > 0.5 ? UNIT_TYPEID::TERRAN_MARINE : UNIT_TYPEID::PROTOSS_ZEALOT;
        //         uint32_t player_id = random > 0.5 ? 1 : 2;
        //         sim.Debug()->DebugCreateUnit(ut, FindRandomLocation({16, 16}, {18, 18}), player_id);
        //     }
        //     //todo set all the units in a low hp

        //     sim.Debug()->SendDebug();
        //     sim.Update();
        //     us = sim.Observation()->GetUnits();
        //     for (const auto& u :us) {
        //         if(u->health!=u->health_max){
        //             std::cout << u->health << "\t" << u->health_max << std::endl;

        //         }
        //     }
        //     std::cout << std::endl;

        //     // for (size_t i = 0; i < 20; i++) {
        //     //     std::cout << "unit number: " << sim.Observation()->GetUnits().size() << std::endl;
        //     //     sim.Update();
        //     //     std::cout << "unit number: " << sim.Observation()->GetUnits().size() << std::endl;
        //     //     debug_renderer.DrawObservation(sim.Observation());
        //     // }
        // }
    }
    {
        // switch (1)
        // {
        // case 1:
        //     std::cout << "1" << std::endl;
        //     std::cout << "1" << std::endl;
        //     break;

        // default:
        //     std::cout << "default" << std::endl;
        //     break;
        // }

        // //! test for the DrawSolution
        // DebugRenderer renderer;
        // //todo start a sim
        // Simulator sim;
        // sim.SetProcessPath("/home/liuyongfeng/StarCraftII/Versions/Base70154/SC2_x64");
        // sim.SetPortStart(3000);
        // sim.SetMapPath("testBattle_distant_vs_melee_debug.SC2Map");
        // sim.LaunchStarcraft();
        // sim.StartGame();
        // //todo kill and create some units belong to me
        // sim.Debug()->DebugKillUnits(sim.Observation()->GetUnits());
        // sim.Debug()->SendDebug();
        // for (size_t i = 0; i < 20; i++)
        // {
        //     sim.Update();
        // }
        // int unit_count = 5;
        // int action_count = 3;
        // GameInfo game_info = sim.Observation()->GetGameInfo();
        // sim.Debug()->DebugCreateUnit(UNIT_TYPEID::TERRAN_MARINE, {25, 25}, 1U, unit_count);
        // sim.Debug()->SendDebug();
        // for (size_t i = 0; i < 2; i++)
        // {
        //     sim.Update();
        // }
        // //todo set the relations from themself to themself
        // Units us = sim.Observation()->GetUnits();
        // std::map<Tag, const Unit*> units_map;
        // for (const auto& u:us )
        // {
        //     units_map[u->tag] = u;
        // }
        // //todo generate some random positions to move for each unit
        // Solution<Command> sol(unit_count);
        // for (size_t i = 0; i < unit_count; i++)
        // {
        //     //todo construct each action series for each unit
        //     Command& command = sol.variable[i];
        //     command.unit_tag = us[i]->tag;
        //     command.actions.resize(action_count);
        //     for (size_t j = 0; j < action_count; j++) {
        //         ActionRaw& action = command.actions[j];
        //         action.target_type = ActionRaw::TargetType::TargetPosition;
        //         action.target_point = FindRandomLocation(game_info);
        //     }
        // }
        // for (size_t i = 0; true; i++)
        // {
        //     sim.Update();
        //     renderer.ClearRenderer();
        //     renderer.DrawObservation(sim.Observation());
        //     renderer.DrawSolution(sol, sim.Observation(), units_map);
        //     renderer.Present();
        // }

        //todo update and draw
    }
    {
        //! test of out of range of map
        // std::map<int, std::string> ismap;
        // ismap[1] = "first";
        // ismap.at(2);
        // std::cout << std::endl;
    }
    {
        //! Ensure if once attack deployed, the units can not take other actions anymore, unless the attack action deployed entirely
        //todo start 
    }

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
    // rolling_bot.SetDebugOn(true);
    // rolling_bot.SetMaxGeneration(10);

    //! participants settings here
    coordinator.SetParticipants({CreateParticipant(Race::Terran, &bot),
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