#include "../project/global_defines.h"

#include <sc2api/sc2_api.h>
#include <sc2api/sc2_score.h>
#include <sc2lib/sc2_utils.h>
#include <vector>
#include <iostream>
#include <list>
#include <chrono>
#include <thread>
#include "../project/rolling_bot/simulator/statistical_data.h"

using namespace sc2;

struct TestEvents
{
    struct Action
    {
        uint32_t game_loop;
        AbilityID ability;
        Action() : game_loop(0), ability(0) {}
        Action(uint32_t loop, AbilityID ab) : game_loop(loop), ability(ab) {}
    };
    struct State
    {
        uint32_t game_loop;
        float state; // shield/health
        State() : game_loop(0), state(0.f){};
        State(uint32_t loop, float st) : game_loop(loop), state(st){};
    };
    struct Position
    {
        uint32_t game_loop;
        Point2D position;
        Position() : game_loop(0), position(Point2D()){};
        Position(uint32_t loop, Point2D pos) : game_loop{loop}, position(pos){};
    };

    std::list<Action> actions; // only self unit
    std::list<State> health;
    std::list<State> shield;
};

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
        Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, FindRandomLocation(Observation()->GetGameInfo()));
        std::cout << Observation()->GetGameLoop() << std::endl;
    }

    virtual void OnUnitDestroyed(const Unit *unit) override
    {
    }

    virtual void OnStep() final
    {
        bool is_order_empty = Observation()->GetUnits(Unit::Alliance::Self).front()->orders.empty();
        std::cout << Observation()->GetGameLoop() << std::endl;
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
    Units m_initial_units;
    std::map<Tag, Unit> m_units_last_loop;
    std::map<Tag, TestEvents> m_events;

private:
    void OnGameEnd() final
    {
    }

    void OnUnitIdle(const Unit *u) final
    {
        Actions()->UnitCommand(u, ABILITY_ID::ATTACK_ATTACK, Observation()->GetUnits(Unit::Alliance::Enemy).front());
        Actions()->UnitCommand(u, ABILITY_ID::MOVE, FindRandomLocation(Observation()->GetGameInfo()), true);
        Actions()->UnitCommand(u, ABILITY_ID::MOVE, Point2D(0.f, GetRandomFraction() * Observation()->GetGameInfo().playable_max.y), true);
        std::cout << "new orders is set at gameloop: " << Observation()->GetGameLoop() << std::endl;
        return;
    }

    void OnGameStart() final
    {
        m_initial_units = Observation()->GetUnits(Unit::Alliance::Self);
        for (auto &&u : m_initial_units)
        {
            m_units_last_loop[u->tag] = *u;
        }

        // some actions
    }

    void OnUnitDestroyed(const Unit *u) final
    {
        m_units_last_loop.erase(u->tag);
    }

    void OnStep()
    {
        Units units = Observation()->GetUnits();
        for (auto &&u : units) // only count alive units
        {
            const Unit &u_last = m_units_last_loop[u->tag];
            //todo record actions
            if (u->orders.empty()) // 1. the oldest order is at 0 2. the order keeps there while it is unfinished 3. process is meaningless in normal unit actions
            {
                if (!u_last.orders.empty())
                {
                    m_events[u->tag].actions.emplace_back(Observation()->GetGameLoop(), u_last.orders.front().ability_id);
                }
            }
            else if (!u_last.orders.empty() && u->orders.front() != u_last.orders.front())
            {
                m_events[u->tag].actions.emplace_back(Observation()->GetGameLoop(), u_last.orders.front().ability_id);
            }
            else if (u_last.weapon_cooldown < u->weapon_cooldown) // attack need to be recorded specially since only if the target has been dead, or the attack order will not be ended
            {
                m_events[u->tag].actions.emplace_back(Observation()->GetGameLoop(), ABILITY_ID::ATTACK_ATTACK);
                std::cout << "attack at loop " << Observation()->GetGameLoop();
                Actions()->SendChat(std::string("attack at loop ") + std::to_string(Observation()->GetGameLoop()));
            }
            // record states
            if (u_last.health != u->health)
            {
                m_events[u->tag].health.emplace_back(Observation()->GetGameLoop(), u->health);
            }
            if (u_last.shield > u->shield) // only record the damaged shield value, since it will increase automatically per frame, the store space will be two large //todo make sure the increase rate of shield
            {
                m_events[u->tag].shield.emplace_back(Observation()->GetGameLoop(), u->shield);
            }
            m_units_last_loop[u->tag] = *u;
        }
        Point2D current_pos = Observation()->GetUnits(Unit::Alliance::Self).front()->pos;
        const std::vector<UnitOrder> &orders = Observation()->GetUnits(Unit::Alliance::Self).front()->orders;
        if (orders.empty())
        {
            std::cout << "orders empty at: " << Observation()->GetGameLoop() << std::endl;
        }
        if (Observation()->GetGameLoop() % 1000 == 0)
        {
            Control()->SaveReplay("tutorial_replay.SC2Replay");
        }
        return;
    }
};

int main(int argc, char *argv[])
{
    // Simulator sim;

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
    coordinator.SetParticipants({CreateParticipant(Race::Terran, &debug_test_bot),
                                 // CreateParticipant(Race::Terran, &enemy_bot),
                                 CreateComputer(Race::Terran)});

    const ObservationInterface *ob = coordinator.GetObservations().front();

    coordinator.LaunchStarcraft();
    coordinator.StartGame("Test.SC2Map");
    // coordinator.StartGame("PCEnemyZealotVSMarinesSim.SC2Map");
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
