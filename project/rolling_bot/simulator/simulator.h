#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <sc2api/sc2_api.h>
#include <sc2api/sc2_proto_interface.h>
#include <map>
#include <string>
#include <vector>
#include "command.h"
#include "debug_renderer/debug_renderer.h"
#include "state.h"

#include <iostream>
#include <queue>

namespace sc2 {
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

class Simulator : public Coordinator {
   public:
    Simulator() {
        SetParticipants(
            {CreateParticipant(Terran, &m_executor), CreateComputer(Terran)});
        SetStepSize(m_step_size);
    }
    ~Simulator() = default;

    //! set your opponent as a user-defined bot
    void SetOpponent(Agent* agent);
    //! set your opponent as a built-in bot - a computer
    void SetOpponent(Difficulty difficulty);

    //! direct send the orders to units
    void SetOrders(const std::vector<Command>& commands, DebugRenderer* debug_renderer = nullptr);
    //! copy the game state from a specific game observation
    void CopyAndSetState(const ObservationInterface* ob, DebugRenderer* debug_renderer = nullptr);
    //! copys state and sets orders for preparation to run
    void SetStartPoint(const std::vector<Command>& commands,
                       const ObservationInterface* ob);
    //! runs for specific number of steps which can be set by user
    void Run(int steps, DebugRenderer* debug_renderer = nullptr);
    //! load the copied state
    void Load();
    //? is anyone really needs a Run() which runs the game until gameover
    //! exposes the whole ObservationInterface to user
    const ObservationInterface* Observation() const;
    //! exposes DebugInterface
    DebugInterface* Debug();
    //! exposes ActionInterface
    ActionInterface* Actions();

    //! Compares current state with the start point to get specific unit group
    //! health loss
    float GetTeamHealthLoss(Unit::Alliance alliance) const;

    const std::map<Tag, const Unit*>& GetRelativeUnits(){
        return m_relative_units;
    }
    
    const State& GetSave(){
        return m_save;
    }

   private:
    //! set units relations
    //! so the caller of this simultor doesn't have to know the tags of units
    //! here
    void SetUnitsRelations(State state, Units us_copied);

    //! simulation step size
    //! if you want the simulation to speed up, you can set a higher value
    int m_step_size = 8;
    //! the bot to be called outside to send orders or get observations
    Executor m_executor;
    //! save of the caller' state when calls it
    State m_initial_save;
    //! save of state
    State m_save;
    //! orders sent in
    std::vector<Command> m_commands;

    //! form resource tag to target unit
    std::map<Tag, const Unit*> m_relative_units;
};

}  // namespace sc2

#endif  // SIMULATOR_H