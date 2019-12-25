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
        Actions()->UnitCommand(unit, ABILITY_ID::ATTACK_ATTACK, enemies.front());
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
        if (Observation()->GetGameLoop() > 100)
        {
            Actions()->UnitCommand(Observation()->GetUnits(Unit::Alliance::Self), ABILITY_ID::STOP_STOP);
        }
        Units team = Observation()->GetUnits(Unit::Alliance::Self);
        for (const auto u : team)
        {
            if (!u->orders.empty())
            {
                std::cout << u->orders.size() << std::endl;
            }
        }
        Units enemies = Observation()->GetUnits(Unit::Alliance::Enemy);
        for (const auto u : enemies)
        {
            if (!u->orders.empty())
            {
                std::cout << u->orders.size() << std::endl;
            }
        }

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

class DebugTestBot : public Agent
{
private:
    Units m_units;
    int m_kill_loop = 1000;
    int m_last_kill_loop;
    std::vector<Point2D> m_pos_set;

private:
    void OnGameEnd() final
    {
        // AgentControl()->Restart();
    }

    void OnUnitIdle(const Unit *u) final
    {
        auto enemies = Observation()->GetUnits();
        Actions()->UnitCommand(u, ABILITY_ID::ATTACK_ATTACK, GetRandomEntry(enemies));
    }

    void OnGameStart() final
    {
        m_units = Observation()->GetUnits(Unit::Alliance::Self);
        Debug()->DebugEnemyControl();
        Debug()->SendDebug();
    }

    void OnStep()
    {
        int loop = Observation()->GetGameLoop();
        Units units = Observation()->GetUnits();
        if (Observation()->GetGameLoop() % m_kill_loop == 0)
        {
            Debug()->DebugKillUnits(Observation()->GetUnits());
            m_last_kill_loop = loop;
        }
        else if (Observation()->GetGameLoop() == m_last_kill_loop + 10)
        {
            for (const Unit *unit : m_units)
            {
                Debug()->DebugCreateUnit(unit->unit_type, unit->pos, unit->owner);
            }
        }
        Debug()->SendDebug();
        //todo see after how many loops the unis number can be zero
        if (Observation()->GetUnits().size() == 0)
        {
            std::cout << "Unit number is 0 at " << Observation()->GetGameLoop() << std::endl;
        }
        else
        {
            std::cout << units.size() << std::endl;
            for (const auto &u : units)
            {
                if (std::find(m_pos_set.begin(), m_pos_set.end(), u->pos) == m_pos_set.end())
                {
                    m_pos_set.push_back(u->pos);
                }
            }
        }
        if (Observation()->GetGameLoop() % 200 == 0)
        {
            std::cout << m_pos_set.size() << std::endl;
            Control()->SaveReplay("./test.SC2Replay");
        }
        Units enemies = Observation()->GetUnits(Unit::Alliance::Enemy);
        if (enemies.size() > 0)
        {
            auto seesee = enemies.front()->orders;
            if (!seesee.empty())
            {
                std::cout << "I can get it" << std::endl;
            }
            auto seesee2 = m_units.front()->orders;
            if (!seesee2.empty())
            {
                std::cout << "get it" << std::endl;
            }
        }
        return;
    }
};

int main(int argc, char *argv[])
{
    Point2D new_p = FixOutsidePointIntoCircle(Point2D(1, -2), Point2D(0, 0), 3);

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
    DebugTestBot debug_test_bot;
    // coordinator.SetMultithreaded(true);
    coordinator.SetParticipants({CreateParticipant(Race::Terran, &bot),
                                 // CreateParticipant(Race::Terran, &enemy_bot),
                                 CreateComputer(Race::Terran)});

    const ObservationInterface *ob = coordinator.GetObservations().front();

    coordinator.LaunchStarcraft();
    coordinator.StartGame("PCEnemyZealotVSMarinesSim.SC2Map");
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
