#include <sc2api/sc2_api.h> // ������������е�api��ͷ�ļ�����Ҳ���˶���
#include "bot_examples.h" 
#include "my_bots/bot_tutor.h"
#include "my_bots/random_bot.h"
#include "my_bots/one_frame_bot.h"
#include <iostream>
#include "my_bots/potential_field/potential_field.h"
#include "my_bots/potential_field/advanced_potential_field_bot.h"
#include "my_bots/rule_based_bots/attack_nearest/attack_nearest.h"

using namespace sc2;

class human_control :public Agent {
    void OnGameStart() {
        initial_units = Observation()->GetUnits();
        Control()->Save();
    }
    void OnStep() {
        if (Observation()->GetGameLoop() % 100 == 0) {
            Control()->Load();
        }
    }
    Units initial_units;
};

class no_action:public Agent {
public:
    no_action() = default;
    ~no_action() = default;
};

int main(int argc, char* argv[]) {
    int step_size = 1;
    // test for runing game repeatedly
    for (size_t i = 0; i < 2; i++) {
        Coordinator coordinator; // coordinator��Э���������������Ϸ�����С�����ǰ�ȵȵ�����
        std::cout << "LoadSettings: " << coordinator.LoadSettings(argc, argv) << std::endl; // ��ȡ���ò���
        coordinator.SetRealtime(false);
        //coordinator.SetRealtime(true);
        coordinator.SetStepSize(step_size); // ������Ϸѭ������
        coordinator.SetMultithreaded(true);


        no_action na_bot;
        Bot bot; // traditional rule-based bot
        random_bot bot2, bot3;
        human_control bot0;
        one_frame_bot of_bot;
        potential_field_bot pf_bot(step_size);
        advanced_potential_field_bot adv_pf_bot(step_size);

        attack_nearest an_bot;

        coordinator.SetParticipants({ // ��ʼ����Ӫô
            //CreateComputer(Race::Terran),
            //CreateParticipant(Race::Terran, &na_bot), // a bot without any actions
            //CreateParticipant(Race::Terran, &bot0), // �������
            CreateParticipant(Race::Terran, &of_bot), // one frame bot
            //CreateParticipant(Race::Terran, &bot3), // random bot
            //CreateParticipant(Race::Terran, &pf_bot), //potential field bot
            //CreateParticipant(Race::Terran, &adv_pf_bot), //advanced potential field bot

            //CreateParticipant(Race::Terran, &an_bot), // ������壬ʹ��Attack Nearest

            CreateComputer(Race::Terran), // ��ӵ��ԣ�Ĭ��easy�Ѷ�
            //CreateParticipant(Race::Terran, &bot2),
            }); // ��Ӳ������

        coordinator.LaunchStarcraft(); // ����Ϸ�������Ĵ���Ϸ�ƺ��յ�Ĭ�ϲ����Ŀ��ƶ�ʲô���涼û�У��ǰ�����Ϊû����Update����

        //coordinator.StartGame(sc2::kMapBelShirVestigeLE); // ��׼�Ծֵ�ͼ
        //coordinator.StartGame("..\\maps\\Test\\testBattle.SC2Map");
        //coordinator.StartGame("..\\maps\\Test\\testBattle_2distant_vs_1melee.SC2Map");
        coordinator.StartGame("..\\maps\\Test\\testBattle_distant_vs_melee.SC2Map");
        //coordinator.StartGame("..\\maps\\Test\\testBattle_distant_vs_distant.SC2Map");
        //coordinator.StartGame("..\\maps\\Test\\testBattle_no_enemy.SC2Map");
        //coordinator.StartGame("..\\maps\\Test\\testBattleAllUnits.SC2Map");
        //coordinator.StartGame("..\\maps\\Test\\testBattle_d_m_vs_d_m.SC2Map");
        //coordinator.StartGame("..\\maps\\Test\\testBattle_m_vs_d.SC2Map");
        //coordinator.StartGame("..\\maps\\Test\\testBattle1v1.SC2Map"); // 1v1��ֹ����


        /*bool is_start = false;
        time_t start = 0;
        int frames = 0;*/
        while (coordinator.Update()) { // run a bot
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

        std::cout << coordinator.AllGamesEnded() << std::endl;
        coordinator.LeaveGame();

    }


    return 0;
}