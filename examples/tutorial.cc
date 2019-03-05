#include <sc2api/sc2_api.h> // ������������е�api��ͷ�ļ�����Ҳ���˶���
#include "bot_examples.h" // ���ʾ��bot�����������bot
#include "my_bots/bot_tutor.h"
#include "my_bots/random_bot.h"
#include "my_bots/one_frame_bot.h"
#include <iostream>
#include "my_bots/potential_field/potential_field.h"

using namespace sc2;

class human_control :public Agent {

};


int main(int argc, char* argv[]) {
    Coordinator coordinator; // coordinator��Э���������������Ϸ�����С�����ǰ�ȵȵ�����
    std::cout << "LoadSettings: " << coordinator.LoadSettings(argc, argv) << std::endl; // ��ȡ���ò���
    coordinator.SetRealtime(true); // ������Ϸ�Ƿ�����ʵ�ٶȽ���
    coordinator.SetStepSize(1); // ������Ϸѭ������

    Bot bot; // bot can only say hello
    random_bot bot2, bot3;
    human_control bot0;
    one_frame_bot of_bot;
    potential_field_bot pf_bot;

    coordinator.SetParticipants({ // ��ʼ����Ӫô
        //CreateComputer(Race::Terran),
        //CreateParticipant(Race::Terran, &bot0), // �������
        //CreateParticipant(Race::Terran, &of_bot), // ������壬ʹ��֮ǰд��AI
        //CreateParticipant(Race::Terran, &bot3), // ������壬ʹ��֮ǰд��AI
        CreateParticipant(Race::Terran, &bot0),
        CreateParticipant(Race::Terran, &pf_bot), // ������壬ʹ��potential field AI

        //CreateComputer(Race::Terran), // ��ӵ��ԣ�Ĭ��easy�Ѷ�
        //CreateParticipant(Race::Terran, &bot2),
        }); // ��Ӳ������

    coordinator.LaunchStarcraft(); // ����Ϸ�������Ĵ���Ϸ�ƺ��յ�Ĭ�ϲ����Ŀ��ƶ�ʲô���涼û�У��ǰ�����Ϊû����Update����

    //coordinator.StartGame(sc2::kMapBelShirVestigeLE); // ��׼�Ծֵ�ͼ
    //coordinator.StartGame("E:\\Desktop\\test\\testBattle.SC2Map");
    coordinator.StartGame("E:\\Desktop\\test\\testBattle_distant_vs_melee.SC2Map");
    //coordinator.StartGame("E:\\Desktop\\test\\testBattle_no_enemy.SC2Map");
    //coordinator.StartGame("E:\\Desktop\\test\\testBattleAllUnits.SC2Map");
    //coordinator.StartGame("E:\\Desktop\\test\\testBattle1v1.SC2Map"); // 1v1��ֹ����


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