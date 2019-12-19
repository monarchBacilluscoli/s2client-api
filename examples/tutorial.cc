#include "../project/global_defines.h"

#include <sc2api/sc2_api.h>
#include <sc2api/sc2_score.h>
#include <sc2lib/sc2_utils.h>
#include <iostream>
#include <list>
#include <chrono>
#include <thread>
#include "debug_renderer/debug_renderer.h"

using namespace sc2;

class EnemyBot : public Agent
{
    void OnGameEnd() final
    {
        AgentControl()->Restart();
    }

    void OnUnitIdle(const Unit *u) final
    {
        Units enemies = Observation()->GetUnits(Unit::Alliance::Enemy);
        Actions()->UnitCommand(u, ABILITY_ID::ATTACK, enemies.front());
    }
};

class Bot : public Agent
{
private:
    Units m_all_units;
    std::list<Unit> m_self_dead_units;
    std::list<Unit> m_ally_dead_units;
    std::list<Unit> m_neutral_dead_units;
    std::list<Unit> m_enemy_dead_units;

public:
    Bot() = default;
    virtual void OnGameStart() final
    {
        m_all_units = Observation()->GetUnits();
        std::cout << __FUNCTION__ << ": " << m_all_units.size() << '\t' << std::flush;
        std::cout << Observation()->GetGameLoop() << '\t' << std::endl;
        // Debug()->DebugKillUnits(Observation()->GetUnits());
        // Debug()->SendDebug();
    }

    virtual void OnGameFullStart() final
    {
        // m_all_units = Observation()->GetUnits();
        // std::cout << m_all_units.size() << '\t' << std::flush;
    }

    virtual void OnUnitIdle(const Unit *unit) final // randomly select an enemy and attack it
    {
        Units enemies = Observation()->GetUnits(Unit::Alliance::Enemy);
        // Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, GetRandomEntry(enemies));
        Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemies.front());
    }

    virtual void OnUnitDestroyed(const Unit *unit) override
    {
        // m_all_units = Observation()->GetUnits();
        // std::cout << m_all_units.size() << std::flush;
        // std::cout << "Player " << unit->owner << "'s unit "
        //           << unit->unit_type << " died" << std::endl;
        // switch (unit->alliance)
        // {
        // case Unit::Alliance::Enemy:
        //     m_self_dead_units.push_back(*unit);
        //     break;
        // case Unit::Alliance::Ally:
        //     m_ally_dead_units.push_back(*unit);
        //     break;
        // case Unit::Alliance::Neutral:
        //     m_neutral_dead_units.push_back(*unit);
        //     break;
        // case Unit::Alliance::Self:
        //     m_self_dead_units.push_back(*unit);
        // default:
        //     break;
        // }
    }

    virtual void OnStep() final
    {
        m_all_units = Observation()->GetUnits();
        std::cout << __FUNCTION__ << ": " << m_all_units.size() << '\t' << std::flush;
        // Debug()->DebugKillUnits(m_all_units);
        std::cout << Observation()->GetGameLoop() << '\t' << std::endl;
        // if (Observation()->GetUnits(Unit::Alliance::Self).size() <= 1 || Observation()->GetUnits(Unit::Alliance::Enemy).size() <= 1)
        // {
        //     std::cout << "ready to end!" << std::endl;
        // }
        return;
    }

    virtual void OnGameEnd() final
    {
        std::cout << "Game ended!" << std::endl;
        std::string path = "replays/last_tutorial.SC2Replay";
        Control()->SaveReplay(path);
        std::cout << "Replay saved" << std::endl;
        std::vector<PlayerResult> game_result = Observation()->GetResults(); // get player result to see if this method is valid
        Score score = Observation()->GetScore();
        OutputGameResult(Observation(), "scores/tutorial_test_scores.txt", "test_remark");
        AgentControl()->Restart(); // it only works in single player game
        return;
    }
};

int main(int argc, char *argv[])
{
#ifdef USE_SYSTEM_COMMAND
    //kill all the previous sc2 processes
    KillProcess("SC2_x64");
#endif
    Coordinator coordinator;
    coordinator.LoadSettings(argc, argv);

    int frames = 60;
#ifdef USE_GRAPHICS
    DebugRenderer renderer;
#endif // USE_GRAPHICS

    Bot bot;
    EnemyBot enemy_bot;
    // coordinator.SetMultithreaded(true);
    coordinator.SetParticipants({CreateParticipant(Race::Terran, &bot),
                                 // CreateParticipant(Race::Terran, &enemy_bot),
                                 CreateComputer(Race::Terran)});

    const ObservationInterface *ob = coordinator.GetObservations().front();

    coordinator.LaunchStarcraft();
    coordinator.StartGame("MazeEnemyTowerVSThors.SC2Map");
    // coordinator.StartGame("Maze2.SC2Map");
#ifdef REAL_TIME_UPDATE
    auto start = std::chrono::steady_clock::now();
    auto end = std::chrono::steady_clock::now();
#endif //REAL_TIME_UPDATE
    while (
#ifdef REAL_TIME_UPDATE
        start = std::chrono::steady_clock::now(),
#endif //REAL_TIME_UPDATE
#ifdef USE_GRAPHICS
        renderer.ClearRenderer(),
        renderer.DrawObservation(ob), // display the game, since StartGame() runs for 1 starting frame, it can not display it by renderer here.
        renderer.Present(),
#endif // USE_GRAPHICS
        coordinator.Update())
    {
#ifdef REAL_TIME_UPDATE
        end = std::chrono::steady_clock::now();
        auto interval = end - start;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / frames) - interval);
#endif // REAL_TIME_UPDATE
    }

    return 0;
}
