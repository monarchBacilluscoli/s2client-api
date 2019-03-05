#include <sc2api/sc2_api.h> // 这里包含了所有的api的头文件，但也仅此而已
#include "bot_examples.h" // 这个示例bot还不抵这里的bot
#include "my_bots/bot_tutor.h"
#include "my_bots/random_bot.h"
#include "my_bots/one_frame_bot.h"
#include <iostream>
#include "my_bots/potential_field/potential_field.h"

using namespace sc2;

class human_control :public Agent {

};


int main(int argc, char* argv[]) {
    Coordinator coordinator; // coordinator（协调器）负责控制游戏进行中、进行前等等的设置
    std::cout << "LoadSettings: " << coordinator.LoadSettings(argc, argv) << std::endl; // 读取设置参数
    coordinator.SetRealtime(true); // 设置游戏是否以真实速度进行
    coordinator.SetStepSize(1); // 设置游戏循环步长

    Bot bot; // bot can only say hello
    random_bot bot2, bot3;
    human_control bot0;
    one_frame_bot of_bot;
    potential_field_bot pf_bot;

    coordinator.SetParticipants({ // 初始化阵营么
        //CreateComputer(Race::Terran),
        //CreateParticipant(Race::Terran, &bot0), // 人类玩家
        //CreateParticipant(Race::Terran, &of_bot), // 添加人族，使用之前写的AI
        //CreateParticipant(Race::Terran, &bot3), // 添加人族，使用之前写的AI
        CreateParticipant(Race::Terran, &bot0),
        CreateParticipant(Race::Terran, &pf_bot), // 添加人族，使用potential field AI

        //CreateComputer(Race::Terran), // 添加电脑，默认easy难度
        //CreateParticipant(Race::Terran, &bot2),
        }); // 添加参与玩家

    coordinator.LaunchStarcraft(); // 打开游戏（这样的打开游戏似乎收到默认参数的控制而什么界面都没有，是啊，因为没有用Update啊）

    //coordinator.StartGame(sc2::kMapBelShirVestigeLE); // 标准对局地图
    //coordinator.StartGame("E:\\Desktop\\test\\testBattle.SC2Map");
    coordinator.StartGame("E:\\Desktop\\test\\testBattle_distant_vs_melee.SC2Map");
    //coordinator.StartGame("E:\\Desktop\\test\\testBattle_no_enemy.SC2Map");
    //coordinator.StartGame("E:\\Desktop\\test\\testBattleAllUnits.SC2Map");
    //coordinator.StartGame("E:\\Desktop\\test\\testBattle1v1.SC2Map"); // 1v1静止测试


    /*bool is_start = false;
    time_t start = 0;
    int frames = 0;*/
    while (coordinator.Update()) { // run a bot
        /*coordinator.LeaveGame();*/
        //? get the frames per second in real-time mode
        /*if (!is_start) {
            is_start = true;
            start = time(NULL);
        }
        else {
            frames++;
            if (time(NULL) - start >= 60) {
                std::cout <<"result : "<< static_cast<float>(frames) / 60 << std::endl;
                frames = 0;
                start = time(NULL);
            }
        }*/
    }


    return 0;
}