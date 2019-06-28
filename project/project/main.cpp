#include <sc2api/sc2_api.h>
#include <liu_renderer/liu_renderer.h>
#include <string>
#include <iostream>
#include <chrono>
#include <thread>

//! test include files
#include "../rolling_bot/simulator/state.h"
#include "../rolling_bot/algorithm/ga.h"
#include "../rolling_bot/algorithm/real_ga.h"

using namespace sc2;

class Bot : public Agent {
public:
    Bot(){
        
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
    LiuRenderer m_renderer;
};

int main(int argc, char* argv[]) {
    //! test code
    //! for real ga
    RealGA real_ga;
    RealGA::SetExampleDimensions(3);
    auto a = &RealGA::example_evaluator;
    real_ga.SetEvaluator(&RealGA::example_evaluator);
    real_ga.SetBoundry(RealGA::example_lower_boundry, RealGA::example_upper_boundry);
    Solution<float> final = real_ga.Run().front();

    // Some settings
    std::string map_path = "testBattle_distant_vs_melee_debug.SC2Map";
    int port_start = 4000;
    bool real_time = false;
    bool multi_threaded = false;
    // use this to control the cauculation times per second
    uint frames = 60;

    Coordinator coordinator;
    coordinator.LoadSettings(argc, argv);

    Bot bot;
    coordinator.SetParticipants({
        CreateParticipant(Race::Terran, &bot),
        CreateComputer(Race::Zerg)
    });
    coordinator.SetPortStart(port_start);
    coordinator.SetRealtime(real_time);
    coordinator.SetMultithreaded(multi_threaded);

    coordinator.LaunchStarcraft();
    coordinator.StartGame(map_path);

    // A fixed time update mechanism
    auto start = std::chrono::steady_clock::now();
    auto end = std::chrono::steady_clock::now();
    while (start = std::chrono::steady_clock::now(), coordinator.Update()) {
        end = std::chrono::steady_clock::now();
        auto interval = end - start;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000/frames)-interval);
    }

    return 0;
}