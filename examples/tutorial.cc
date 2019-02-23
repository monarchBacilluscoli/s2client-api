#include <sc2api/sc2_api.h> // ������������е�api��ͷ�ļ�����Ҳ���˶���
#include "bot_examples.h" // ���ʾ��bot�����������bot
#include "my_bots/bot_tutor.h"
#include "my_bots/random_bot.h"
#include <iostream>

using namespace sc2;

class human_control :public Agent {

};


int main(int argc, char* argv[]) {
    Coordinator coordinator; // coordinator��Э���������������Ϸ�����С�����ǰ�ȵȵ�����
    std::cout << "LoadSettings: " << coordinator.LoadSettings(argc, argv) << std::endl; // ��ȡ���ò���
    coordinator.SetRealtime(false); // ������Ϸ�Ƿ�����ʵ�ٶȽ���
    coordinator.SetStepSize(1); // ������Ϸѭ������

    Bot bot; // ���Ǹ�д���Ǹ�ֻ��hello�Ļ�����
    random_bot bot2, bot3;
    human_control bot0;
    coordinator.SetParticipants({ // ��ʼ����Ӫô
        //CreateComputer(Race::Terran),
        //CreateParticipant(Race::Terran, &bot0), // �������
        CreateParticipant(Race::Terran, &bot2), // ������壬ʹ��֮ǰд��AI
        //CreateParticipant(Race::Terran, &bot3), // ������壬ʹ��֮ǰд��AI

        CreateComputer(Race::Terran), // ��ӵ��ԣ�Ĭ��easy�Ѷ�
        //CreateParticipant(Race::Terran, &bot2),
        }); // ��Ӳ������

    coordinator.LaunchStarcraft(); // ����Ϸ�������Ĵ���Ϸ�ƺ��յ�Ĭ�ϲ����Ŀ��ƶ�ʲô���涼û�У��ǰ�����Ϊû����Update����

    //coordinator.StartGame(sc2::kMapBelShirVestigeLE); // ��׼�Ծֵ�ͼ
    //coordinator.StartGame("E:\\Desktop\\test\\testBattle.SC2Map");
    coordinator.StartGame("E:\\Desktop\\test\\testBattle_distant_vs_melee.SC2Map");
    //coordinator.StartGame("E:\\Desktop\\test\\testBattle_no_enemy.SC2Map");
    //coordinator.StartGame("E:\\Desktop\\test\\testBattleAllUnits.SC2Map");
    //coordinator.StartGame("E:\\Desktop\\test\\testBattle1v1.SC2Map"); // 1v1��ֹ����

    while (coordinator.Update()) { // run a bot
        coordinator.LeaveGame();
    }


    return 0;
}