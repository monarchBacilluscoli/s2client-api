#include <sc2api/sc2_api.h>

#include <iostream>
#include <list>

using namespace sc2;

class Bot : public Agent
{
private:
public:
    Bot() = default;
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
        switch (unit->alliance)
        {
        case Unit::Alliance::Enemy:
            m_self_dead_units.push_back(*unit);
            break;
        case Unit::Alliance::Ally:
            m_ally_dead_units.push_back(*unit);
            break;
        case Unit::Alliance::Neutral:
            m_neutral_dead_units.push_back(*unit);
            break;
        case Unit::Alliance::Self:
            m_self_dead_units.push_back(*unit);
        default:
            break;
        }
    }

    virtual void OnStep() final
    {
        Units units = Observation()->GetUnits();
        std::cout << units.size() << "\t" << std::flush;

    }

    virtual void OnGameEnd() final
    {
        std::cout<<"Game Ended!"<<std::endl;
        std::string path = "replays/last_replay.SC2Replay";
        Control()->SaveReplay(path);
        AgentControl()->Restart();
    }

    std::list<Unit> m_self_dead_units;
    std::list<Unit> m_ally_dead_units;
    std::list<Unit> m_neutral_dead_units;
    std::list<Unit> m_enemy_dead_units;
};

int main(int argc, char *argv[])
{
    Coordinator coordinator;
    coordinator.LoadSettings(argc, argv);

    Bot bot;
    coordinator.SetParticipants({CreateParticipant(Race::Terran, &bot),
                                 CreateComputer(Race::Terran)});

    coordinator.LaunchStarcraft();
    coordinator.StartGame("EnemyTowerVSThorsTestMechanismFake.SC2Map");

    int current_loop = 0;
    while (coordinator.Update())
    {
        ++current_loop;
        if (current_loop == 10)
        {
            coordinator.LeaveGame();
            bool ended = coordinator.AllGamesEnded();
            continue;
        }
    }

    return 0;
}
