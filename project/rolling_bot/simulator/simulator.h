#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "../../global_defines.h"

#include "debug_renderer/debug_renderer.h"
#include "state.h"
#include "command.h"
#include <sc2api/sc2_api.h>
#include <map>
#include <string>
#include <thread>
#include <queue>
#include <list>
#include <iostream>

namespace sc2
{
struct UnitsByAlliance
{
    std::list<Unit> self{}, ally{}, enemy{}, neutral{};
};

class Executor : public Agent
{
private:
    std::map<Tag, std::queue<ActionRaw>> m_commands;
    std::map<Tag, float> m_cooldown_last_frame;
    bool m_is_setting = true; // if in setting state, do not call any of the client event here
    UnitsByAlliance m_dead_units;

public:
    // transform vector commands to map+queue commands for easy use
    void SetCommands(const std::vector<Command> &commands);
    void Initialize();
    void Clear();
    void SetIsSetting(bool is_setting);

    void OnStep() override;
    void OnUnitDestroyed(const Unit *unit) override;

    const std::list<Unit> &GetDeadUnits(Unit::Alliance alliance) const;

private:
    void ClearCooldownData();
    void ClearCommands();
    void ClearDeadUnits();
};

class Simulator : public Coordinator
{

public:
    Simulator()
    {
        SetParticipants(
            {CreateParticipant(Terran, &m_executor), CreateComputer(Terran)});
    }
    ~Simulator() = default;

    //! set your opponent as a user-defined bot
    void SetOpponent(Agent *agent);
    //! set your opponent as a built-in bot - a computer
    void SetOpponent(Difficulty difficulty);

    //! copys state and sets orders for preparation to run
    void SetStartPoint(const std::vector<Command> &commands, const ObservationInterface *ob);
    //! direct send the orders to units
    void SetOrders(const std::vector<Command> &commands
#ifdef USE_GRAPHICS
                   ,
                   DebugRenderer *debug_renderer = nullptr
#endif // USE_GRAPHICS
    );
    //! get the stored orders (tag-translated)
    //! no unit tag translation, use the local unit tags
    void SetDirectOrders(const std::vector<Command> &commands
#ifdef USE_GRAPHICS
                         ,
                         DebugRenderer *debug_renderer = nullptr
#endif // USE_GRAPHICS
    );
    //! copy the game state from a specific game observation.
    void CopyAndSetState(const ObservationInterface *ob
#ifdef USE_GRAPHICS
                         ,
                         DebugRenderer *debug_renderer = nullptr
#endif // USE_GRAPHICS
    ); // the debug_renderer was only used to debug this function when I worte it.
    //! runs for specific number of steps which can be set by user
    std::thread::id Run(int steps
#ifdef USE_GRAPHICS
                        ,
                        DebugRenderer *debug_renderer = nullptr
#endif // USE_GRAPHICS
    );

    //! load the copied state
    void Load();
    //? is anyone really needs a Run() which runs the game until gameover
    //! exposes ObservationInterface of the executor
    const ObservationInterface *Observation() const;
    //! exposes DebugInterface of the executor
    DebugInterface *Debug();
    //! exposes ActionInterface of the executor
    ActionInterface *Actions();
    ControlInterface *Control();

    std::vector<Command> GetOrders() { return m_commands; }
    std::vector<Command> GetOriginalOrders() { return m_original_commands; } // get the original orders (sent and stored, raw orders)
    float GetTeamHealthLoss(Unit::Alliance alliance) const;                  // get health loss result
    const std::list<Unit> &GetTeamDeadUnits(Unit::Alliance alliance) const;  // get dead units result

    const std::map<Tag, const Unit *> &GetRelativeUnits() { return m_relative_units; }
    const State &GetSave() { return m_save; }

private:
    //! set units relations
    //! so the caller of this simultor doesn't have to know the tags of units
    //! here
    void SetUnitsRelations(State state, Units us_copied);

    //! the bot to be called outside to send orders or get observations
    Executor m_executor;
    //! save of the caller' state when calls it
    State m_initial_save;
    //! save of state
    State m_save;
    //! orders sent in
    std::vector<Command> m_original_commands;
    //! translated orders by local unit tags rather than original tags
    std::vector<Command> m_commands;

    //! form resource tag to target unit
    std::map<Tag, const Unit *> m_relative_units;

public:
    static std::string GetSimMapPath(const std::string &map_path);
};

} // namespace sc2

#endif // SIMULATOR_H