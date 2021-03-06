#include <sc2api/sc2_api.h> // All the head files
#include <set>
#include "bot_examples.h" 
#include "my_bots/bot_tutor.h"
#include "my_bots/random_bot.h"
#include "my_bots/one_frame_bot.h"
#include <iostream>
#include "my_bots/potential_field/potential_field.h"
#include "my_bots/potential_field/advanced_potential_field_bot.h"
#include "my_bots/rule_based_bots/attack_nearest/attack_nearest.h"
#include "my_bots/tests/step_size_test_bot.h"
#include "my_bots/tests/debug_test_bot.h"
#include "my_bots/tests/remote_draw.h"
#include "my_bots/RollingBot/simulator.h"
//#include "my_bots/tests/simulator_test.h"
//#include "my_bots/Algorithm/real_GA.h"
#include "my_bots/RollingBot/rolling_bot.h"
#include "..///liu_renderer/liu_renderer/liu_renderer.h"
//#include <sc2utils/ssh_connection.h>

using namespace sc2;

const int kFeatureLayerSize = 80;
const float kCameraWidth = 24.0f;

//? for test, delete it
float evaluator(std::vector<float> x) {
    return 1 - pow(x[0], 2) - pow(x[1], 2);
}

class human_control :public Agent {
public:
    human_control(Coordinator& coordinator) :m_coordinator(coordinator) {}
    void OnGameStart() {
        Units us = Observation()->GetUnits();
        std::for_each(us.begin(), us.end(), [&](const Unit*& u) {
            Debug()->DebugSetShields(u->shield_max / 2, u);
            });
        Debug()->SendDebug();
    }
    void OnStep() {
        if (Observation()->GetGameLoop() > 5) {
            Units us = Observation()->GetUnits();
            std::for_each(us.begin(), us.end(), [&](const Unit*& u) {
                Debug()->DebugSetShields(0, u);
                });
            Debug()->SendDebug();
        }
    }
    State m_save;
    int m_live_loops = 20;
    int m_current_live_loops = 0;
    Coordinator& m_coordinator;
};

// Haha, here is a bot with no real code
class no_action:public Agent {};

int main(int argc, char* argv[]) {

    //// try libssh in this project to ensure I have set it up already
    std::string net_address = "59.71.231.175";
    std::string username = "liuyongfeng";
    std::string password = "1121";
    //! you should set the absolute directory, or it doesn't work
    std::string process_path = "StarCraftII/Versions/Base70154/SC2_x64";
    std::string execution = "StarCraftII/Versions/Base70154/SC2_x64 -listen 59.71.231.175:3000";
    int port_start = 3000;

    //SSHConnection connection(net_address);
    //connection.Connect(username, password);
    //connection.Execute(execution);

    // Before doing anything, you should set it first
    bool isCoonected = SetSSHConnection(net_address, username, password);

   /* Simulator sim; 
    sim.SetNetAddress(net_address);
    sim.SetOpponent(Difficulty::Easy);
    sim.SetPortStart(3000);
    sim.SetRealtime(false);
    sim.SetProcessPath("StarCraftII/Versions/Base70154/SC2_x64");
    sim.LaunchRemoteStarcraft();
    sim.StartGame("..\\Maps\\testBattle_distant_vs_melee_debug.SC2Map");
    sim.Update();
    Units us = sim.Observation()->GetUnits();*/


    int step_size = 1;
    // test for runing game repeatedly
    for (size_t i = 0; i < 2; i++) {
        Coordinator coordinator; // （协调器）负责控制游戏进行中、进行前等等的设置
        std::cout << "isLoadSettings: " << coordinator.LoadSettings(argc, argv) << std::endl; // If there is no command line arguments it will check the files in your MyDocument\StarCraft II and get default settings.
        std::cout<<"Executable path: " <<coordinator.GetExePath()<<std::endl;
        coordinator.SetRealtime(false);
        //coordinator.SetRealtime(true);
        coordinator.SetStepSize(step_size); // 设置游戏循环步长
        coordinator.SetMultithreaded(true);

        coordinator.SetPortStart(4000);

        no_action na_bot;
        Bot bot; // traditional rule-based bot
        random_bot bot2, bot3;
        human_control bot0(coordinator);
        one_frame_bot of_bot, of_bot2;
        potential_field_bot pf_bot(step_size);
        advanced_potential_field_bot adv_pf_bot(step_size);
        step_size_test_bot sstb;
        debug_test_bot dtb; //! This is a nightmare for all units in all maps, just start screaming!
        remote_draw rdb;
        //rolling_bot rolling_bot(net_address, port_start, process_path, "..\\Maps\\testBattle_distant_vs_melee_debug.SC2Map");

        attack_nearest an_bot;

        coordinator.SetParticipants({ // 初始化阵营么
            //CreateComputer(Race::Terran),
            //CreateParticipant(Race::Terran, &na_bot), // a bot without any actions
            //CreateParticipant(Race::Terran, &bot0), // 人类玩家
            //CreateParticipant(Race::Terran, &of_bot), // one frame bot
            //CreateParticipant(Race::Terran, &bot3), // random bot
            //CreateParticipant(Race::Terran, &pf_bot), //potential field bot
            //CreateParticipant(Race::Terran, &adv_pf_bot), //advanced potential field bot
            //CreateParticipant(Race::Terran, &sstb),
            CreateParticipant(Race::Terran, &an_bot),

            //CreateParticipant(Race::Terran, &an_bot), // 添加人族，使用Attack Nearest

            CreateComputer(Race::Terran), // 添加电脑，默认easy难度
            //PlayerSetup(Observer,Terran,&na_bot)
            //CreateParticipant(Race::Terran, &bot2),
            }); // 添加参与玩家

        //? Here is Port
        coordinator.LaunchStarcraft();

        //! made it
        //sc2::FeatureLayerSettings settings(kCameraWidth, kFeatureLayerSize, kFeatureLayerSize, kFeatureLayerSize, kFeatureLayerSize);
        //coordinator.SetFeatureLayers(settings);
        //coordinator.SetupPorts(2, 3001);
        //coordinator.SetNetAddress("59.71.231.175");
        //coordinator.Connect(3000);
        //! if the remote client was used, the path should be set properly
        //! When StartGame() is called, it'll call OnGameStart() and OnStep() of each client once
        //coordinator.StartGame("..\\Maps\\testBattle_distant_vs_melee_debug.SC2Map");

        //coordinator.StartGame(sc2::kMapBelShirVestigeLE); // 标准对局地图
        //coordinator.StartGame("Test\\testBattle.SC2Map");
        //coordinator.StartGame("Test\\testBattle_2distant_vs_1melee.SC2Map");

        coordinator.StartGame("Test\\testBattle_distant_vs_melee_debug.SC2Map");
        //coordinator.StartGame("Test\\testBattle_distant_vs_distant.SC2Map");
        //coordinator.StartGame("Test\\testBattle_distant_vs_distant.SC2Map");
        //coordinator.StartGame("Test\\testBattle_no_enemy.SC2Map");
        //coordinator.StartGame("Test\\testBattleAllUnits.SC2Map");
        //coordinator.StartGame("Test\\testBattle_d_m_vs_d_m.SC2Map");
        //coordinator.StartGame("Test\\testBattle_m_vs_d.SC2Map");
        //coordinator.StartGame("Test\\testBattle1v1.SC2Map"); // 1v1静止测试
        //coordinator.StartGame("Test\\testMechanism_StepSize.SC2Map");

        
        while (coordinator.Update()) { // run a bot
            //if (bot0.Observation()->GetGameLoop() == 100) {
            //    bot0.m_save = SaveMultiPlayerGame(bot0.Observation());
            //}
            //if (bot0.Observation()->GetGameLoop() > 100) {
            //    LoadMultiPlayerGame(bot0.m_save, bot0, coordinator);
            //}
            // Sleeps for 64 mileseconds can make the unrealtime game easy to observe

            SleepFor(64);
        }
        //todo at the end of a game set, I need to output the results of the game
        
    }


    return 0;
}