/*! \file simulator.h
    \brief Frontend for running a simulator, actually, a remote game.

    The coordinator acts as a remote game manager. It is used to remotely attach to a StarCraft and setup proto connections
    between a user's bot and the running StarCraft instance. And it provided function of copying the current game settings and state so that let the remote instance run as a simulator for current game
*/

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include<sc2api/sc2_api.h>
#include"../my_bots/solution.h"

namespace sc2 {

    //? Maybe I don't need this bot to have any code, I can execute the order from outside
    class Executor: public Agent
    {
    public:
        //void OnGameStart() final;
        void OnStep() final;
    };

    class Simulator
    {
    public:
        //todo use a constructor to copy current state and game settings. And of course you have to provide the remote port and IP.
        //todo 
        Simulator();
        ~Simulator();

        //todo 
        void SetFeatureLayers(const FeatureLayerSettings& settings);
        //todo
        void SetOpponent(const std::vector<PlayerSetup>& opponent);
        //todo how long, use what to count? time? frames? or process of game?
        void SetStarPoint(command command, ObservationInterface* ob);
        void Run(int frames);
    private:
    //todo give the order, did I need to store the order?
        void SetOrder(command command);
    //todo give the state
        void SetState(ObservationInterface* ob);
    //todo I think I need to store the pointer to the Bot, and 

        Executor m_exector;
        CoordinatorImp* imp_;
    };

    //Simulator::Simulator()
    //{
        //? What settings should be copied from coordinator(map) and what should not(ip, ports)
        //? What settings can be changed after construction and what should not(almost everything can be fixed in old Coordinator)
        //todo Check if there are something useful in LoadSettings
        //todo Multithreaded??
        //todo set realtime false
        //todo Maybe I can check the process path
        //todo set the bot which can deploy my orders, as for the other one...
        //todo users can set the opponent here to, or set it after initialization
        //todo no launch, but need to setup ports and connect
        //todo start game(map)
        //todo create game(open a room for clients, clients not be connected yet)
        //todo join game
    //}

}

#endif // !SIMULATOR_H
