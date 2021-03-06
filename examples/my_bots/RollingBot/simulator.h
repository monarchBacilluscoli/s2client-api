/*! \file simulator.h
    \brief Frontend for running a simulator, actually, a remote game.

    The coordinator acts as a remote game manager. It is used to remotely attach to a StarCraft and setup proto connections
    between a user's bot and the running StarCraft instance. And it provided function of copying the current game settings and state so that let the remote instance run as a simulator for current game
    Note that if you want to use this method correctly, you need to have a entirely exposed map or a bot with full full observation
*/

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <string>
#include <vector>
#include <map>
#include <sc2api/sc2_api.h>
#include <sc2api/sc2_proto_interface.h>
#include "../my_bots/solution.h"
#include "../my_bots/tests/remote_draw.h"
//#include "../my_bots/utilities/ssh_connection.h"

#include "state.h"

namespace sc2 {

    //! An empty bot to be called outside 
    class Executor : public Agent {};

    class Simulator : public Coordinator
    {
    public:
        Simulator() {
            SetParticipants({
                CreateParticipant(Terran, &m_executor),
                CreateComputer(Terran)
                }
            );
            SetStepSize(m_step_size);
        }
        ~Simulator() = default;

        //! set your opponent as a user-defined bot
        void SetOpponent(Agent* agent);
        //! set your opponent as a built-in bot - a computer
        void SetOpponent(Difficulty difficulty);

        //! direct send the orders to units
        void SetOrders(std::vector<Command> commands);
        //! copy the game state from a specific game observation
        void CopyAndSetState(const ObservationInterface* ob);
        //! copys state and sets orders for preparation to run
        void SetStartPoint(std::vector<Command> commands, const ObservationInterface* ob);
        //! runs for specific number of steps which can be set by user
        void Run(int steps);
        //! load the copied state
        void Load();
        //? is anyone really needs a Run() which runs the game until gameover
        //! exposes the whole ObservationInterface to user
        const ObservationInterface* Observation() const;

        int GetStepSize() const {
            return m_step_size;
        }
        //! Compares current state with the start point to get specific unit group health loss
        float GetTeamHealthLoss(Unit::Alliance alliance) const;
    private:
        //! set units relations
        //! so the caller of this simultor doesn't have to know the tags of units here
        void SetUnitsRelations(State state, Units us_copied);

        //! simulation step size
        //! if you want the simulation to speed up, you can set a higher value
        int m_step_size = 8;
        //! it is based on the participant passed
        //bool m_is_multi_player = false;
        //! the bot to be called outside to send orders or get observations
        Executor m_executor;
        //! save of the caller' state when calls it
        State m_initia_save;
        //! save of state
        State m_save;

        //! form resource tag to target unit
        std::map<Tag, const Unit*> m_relative_units;
    };
}

#endif // !SIMULATOR_H
