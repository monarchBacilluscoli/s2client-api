#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <sc2api/sc2_api.h>
#include <sc2api/sc2_proto_interface.h>
#include <map>
#include <string>
#include <vector>
#include "debug_renderer/debug_renderer.h"
#include "state.h"
#include <thread>
#include <mutex>
#include <future>
#include "command.h"

#include <iostream>
#include <queue>

namespace sc2
{
class Executor : public Agent
{
    //! for test
    // void OnUnitDestroyed(const Unit* u) override {
    //     // std::cout << "Destroyed Unit:\t " << u->unit_type << "\t" << "("<< u->pos.x << "," <<u->pos.y<< ")" << "\t" << u->health << std::endl;
    // }
public:
    // transform vector commands to map+queue commands for easy use
    void SetCommands(const std::vector<Command> &commands);
    void ClearCooldownData();
    void ClearCommands();
    void SetIsSetting(bool is_setting);

    void OnStep() override;
    // void OnUnitDestroyed(const Unit *u) override;

private:
    std::map<Tag, std::queue<ActionRaw>> m_commands;
    std::map<Tag, float> m_cooldown_last_frame;
    bool m_is_setting = true; // if in setting state, do not call any of the client event here
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

    //! direct send the orders to units
    void SetOrders(const std::vector<Command> &commands, DebugRenderer *debug_renderer = nullptr);
    //! get the stored orders (tag-translated)
    std::vector<Command> GetOrders() { return m_commands; }
    //! get the original orders (sent and stored, raw orders)
    std::vector<Command> GetOriginalOrders() { return m_original_commands; }
    //! no unit tag translation, use the local unit tags
    void SetDirectOrders(const std::vector<Command> &commands, DebugRenderer *debug_renderer = nullptr);
    //! copy the game state from a specific game observation.
    // the debug_renderer was only used to debug this function when I worte it.
    void CopyAndSetState(const ObservationInterface *ob, DebugRenderer *debug_renderer = nullptr);
    //! copys state and sets orders for preparation to run
    void SetStartPoint(const std::vector<Command> &commands,
                       const ObservationInterface *ob);
    //! runs for specific number of steps which can be set by user
    std::thread::id Run(int steps, DebugRenderer *debug_renderer = nullptr);
    //! load the copied state
    void Load();
    //? is anyone really needs a Run() which runs the game until gameover
    //! exposes the whole ObservationInterface to user
    const ObservationInterface *Observation() const;
    //! exposes DebugInterface
    DebugInterface *Debug();
    //! exposes ActionInterface
    ActionInterface *Actions();
    // std::thread::id iddd;

    //! Compares current state with the start point to get specific unit group
    //! health loss
    float GetTeamHealthLoss(Unit::Alliance alliance) const;

    const std::map<Tag, const Unit *> &GetRelativeUnits()
    {
        return m_relative_units;
    }

    const State &GetSave()
    {
        return m_save;
    }

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
};

struct Simulators
{
public:
    Simulators() = default;
    Simulators(int size, const std::string &net_address, int port_start, const std::string &process_path, const std::string &map_path) : simulators(size),
                                                                                                                                         m_net_address(net_address),
                                                                                                                                         m_port_start(port_start),
                                                                                                                                         m_process_path(process_path),
                                                                                                                                         m_map_path(map_path)
    {
        for (Simulator &sim : simulators)
        {
            //sim.SetNetAddress(m_net_address);
            sim.SetPortStart(port_start);
            sim.SetProcessPath(process_path);
            sim.SetMapPath(map_path);
            sim.SetStepSize(1);
            port_start += 2;
        }
    }
    void StartAsync()
    {
        std::vector<std::future<void>> futures(simulators.size());
        for (size_t i = 0; i < futures.size(); i++)
        {
            futures[i] = std::async(std::launch::async, [&, i]() -> void {
                simulators[i].LaunchStarcraft();
                simulators[i].StartGame();
            });
        }
    }

    //simulators
    std::vector<Simulator> simulators;

    // settings
    std::string m_net_address;
    int m_port_start;
    std::string m_process_path;
    std::string m_map_path;
};

} // namespace sc2

#endif // SIMULATOR_H