#include "../project/global_defines.h"

#include <sc2api/sc2_api.h>
#include <sc2api/sc2_score.h>
#include <sc2lib/sc2_utils.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <list>
#include <chrono>
#include <thread>
#include "../project/rolling_bot/simulator/statistical_data.h"

using namespace sc2;

std::string CurrentFolder()
{
    std::string path(__FILE__);
    std::size_t cut = path.rfind('/');
    return path.substr(0, cut);
}

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
        std::string path = "sc2replays/last_tutorial.SC2Replay";
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

class CopyTestBot : public Agent
{
    std::vector<Unit> m_save;

public:
    void OnGameStart() final
    {
        auto us = Observation()->GetUnits();
        m_save.clear();
        for (auto &&u : us)
        {
            m_save.push_back(*u);
        }
        return;
    }
    void OnStep() final
    {
        if (Observation()->GetGameLoop() % 200 == 0 && Observation()->GetGameLoop() != 0)
        {
            auto us = Observation()->GetUnits();
            m_save.clear();
            for (auto &&u : us)
            {
                m_save.push_back(*u);
            }
        }
        if (Observation()->GetGameLoop() % 200 == 2 && Observation()->GetGameLoop() != 0)
        {
            Debug()->DebugKillUnits(Observation()->GetUnits());
            Debug()->SendDebug();
        }
        if (Observation()->GetGameLoop() % 200 == 4 && Observation()->GetGameLoop() != 2)
        {
            for (auto &&u : m_save)
            {
                Debug()->DebugCreateUnit(u.unit_type, u.pos, u.owner == 16 ? 0 : u.owner);
            }
            Debug()->SendDebug();
        }
        if (Observation()->GetGameLoop() % 200 == 6 && Observation()->GetGameLoop() != 4)
        {
            std::cout << Observation()->GetUnits().size() << std::endl;
        }
        return;
    }
};

class FacingTestBot : public Agent
{
private:
    std::vector<float> facing_record = std::vector<float>(10000);
    const Unit *tank;

public:
    void OnGameStart()
    {
        tank = Observation()->GetUnits(Unit::Alliance::Enemy).front();
    }
    void OnStep()
    {
        if (Observation()->GetGameLoop() < facing_record.size())
        {
            facing_record[Observation()->GetGameLoop()] = tank->facing;
        }
        else if (Observation()->GetGameLoop() == facing_record.size())
        {
            std::string record_path = std::string(__FILE__);
            size_t pos = record_path.rfind('/');
            std::fstream output_file(record_path.substr(0, pos) + "/my_test_data/facing_data.txt", std::ios::out | std::ios::trunc);
            output_file << std::endl;
            for (size_t i = 0; i < facing_record.size(); i++)
            {
                output_file << facing_record[i] << '\t';
            }
            output_file << std::endl;
        }
        else
        {
            std::cout << "okk!" << std::endl;
        }
    }
};

class ActionTestBot : public Agent
{
private:
    std::map<Tag, Unit> m_units_states_last_loop;
    std::map<Tag, bool> m_is_an_attack_to_be_recorded;
    bool m_is_left = true;
    int target_loc = 1;

public:
    void OnStep() override
    {
        // execute
        for (const Unit *u : Observation()->GetUnits(Unit::Alliance::Self))
        {
            bool has_cooldown_record = (m_units_states_last_loop.find(u->tag) != m_units_states_last_loop.end());
            // check if the current has been finished
            if (u->orders.empty() ||                                                                                // no order now/ start
                (has_cooldown_record && (m_units_states_last_loop[u->tag].weapon_cooldown < u->weapon_cooldown)) || // this unit has executed a new attack just now
                (!has_cooldown_record && u->weapon_cooldown > 0.f))                                                 // it has shot once but that wasn't recorded
            {
                Point2D target_point;
                if (target_loc == 1)
                {
                    target_point.x = 12;
                    target_point.y = 16;
                    target_loc = 2;
                }
                else if (target_loc == 2)
                {
                    target_point.x = 16;
                    target_point.y = 20;
                    target_loc = 3;
                }
                else if (target_loc == 3)
                {
                    target_point.x = 16;
                    target_point.y = 25;
                    target_loc = 4;
                }
                else if (target_loc == 4)
                {
                    target_point.x = 16;
                    target_point.y = 25.1;
                    target_loc = 1;
                }
                Actions()->UnitCommand(u, ABILITY_ID::MOVE, target_point);
                Actions()->UnitCommand(u, ABILITY_ID::ATTACK, target_point, true);
            }
        }
        // data
        std::fstream fs(CurrentFolder() + "/my_test_data/" + "test.txt", std::ios::out | std::ios::app);
        fs << Observation()->GetGameLoop() << '\t';
        for (const Unit *u : Observation()->GetUnits(Unit::Alliance::Self))
        {
            fs << u->pos.x << '\t' << u->pos.y << '\t' << u->orders.size() << ':' << '\t';
            for (const auto &order : u->orders)
            {
                fs << order.ability_id << '\t' << order.target_pos.x << '\t' << order.target_pos.y << '\t' << order.target_unit_tag << '\t';
            }
        }
        fs << std::endl;
        // my note method
        std::fstream record_fs(CurrentFolder() + "/my_test_data/" + "record.txt", std::ios::out | std::ios::app);
        for (const Unit *u : Observation()->GetUnits(Unit::Alliance::Self))
        {
            if (m_units_states_last_loop.find(u->tag) != m_units_states_last_loop.end())
            {
                const Unit &u_last = m_units_states_last_loop[u->tag];
                if ((u->orders.empty() && !u_last.orders.empty()) || (!u_last.orders.empty() && u->orders.front() != u_last.orders.front()))
                {
                    if (u_last.orders.front().ability_id == ABILITY_ID::ATTACK && m_is_an_attack_to_be_recorded.find(u->tag) != m_is_an_attack_to_be_recorded.end() && m_is_an_attack_to_be_recorded.at(u->tag) == true) // this attack has been executed successful.
                    {
                        record_fs << Observation()->GetGameLoop() - 1 << '\t' << u_last.pos.x << '\t' << u_last.pos.y << '\t' << 32 << '\t' << u_last.orders.front().target_pos.x << '\t' << u_last.orders.front().target_pos.y << '\t' << std::endl;
                        m_is_an_attack_to_be_recorded.at(u->tag) = false;
                    }
                    else // not an attack action or attack doesn't take effect (no enemy)
                    {
                        record_fs << Observation()->GetGameLoop() - 1 << '\t' << u_last.pos.x << '\t' << u_last.pos.y << '\t' << u_last.orders.front().ability_id << '\t' << u_last.orders.front().target_pos.x << '\t' << u_last.orders.front().target_pos.y << '\t' << std::endl;
                    }
                }
            }
            else
            {
                record_fs << Observation()->GetGameLoop() << '\t' << u->pos.x << '\t' << u->pos.y << '\t' << -1 << std::endl;
            }
        }
        // record
        if (Observation()->GetGameLoop() == 1000)
        {
            Control()->SaveReplay(CurrentFolder() + '/' + "agent_replay.SC2Replay");
        }
        // update the record
        for (const Unit *u : Observation()->GetUnits(Unit::Alliance::Self))
        {
            m_units_states_last_loop[u->tag] = *u;
        }
    }
};

class SaveTestBot : public Agent
{
private:
    std::vector<Unit> us;

public:
    void OnStep()
    {
        std::cout << Observation()->GetGameLoop() << std::endl;
        if (Observation()->GetGameLoop() == 100)
        {
            Control()->Save();
            for (const auto &u : Observation()->GetUnits())
            {
                us.push_back(*u);
            }
        }
        if (Observation()->GetGameLoop() % 400 == 0 && Observation()->GetGameLoop() != 0)
        {
            Control()->Load();
            //check the unit
            for (const auto &u : Observation()->GetUnits())
            {
                std::cout << u->tag << 't';
            }
            std::cout << std::endl;
        }
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
    DebugTestBot debug_test_bot;
    CopyTestBot copy_test_bot;
    FacingTestBot facing_test_bot;
    ActionTestBot action_test_bot;
    SaveTestBot save_bot;
    // coordinator.SetMultithreaded(true);
    coordinator.SetParticipants({CreateParticipant(Race::Terran, &save_bot),
                                 // CreateParticipant(Race::Terran, &enemy_bot),
                                 CreateComputer(Race::Terran)});

    const ObservationInterface *ob = coordinator.GetObservations().front();

    coordinator.LaunchStarcraft();
    coordinator.StartGame("/home/liuyongfeng/StarCraftII/maps/Test/ActionTest.SC2Map");
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
