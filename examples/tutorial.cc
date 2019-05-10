#include <sc2api/sc2_api.h> // All the head files
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
    //? print the argvs and see what they are...
    for (int i = 0; i < argc; i++) {
        std::cout << argv[i] << std::endl;
    }

    int step_size = 100;
    // test for runing game repeatedly
    for (size_t i = 0; i < 2; i++) {
        Coordinator coordinator; // coordinator��Э���������������Ϸ�����С�����ǰ�ȵȵ�����
        std::cout << "isLoadSettings: " << coordinator.LoadSettings(argc, argv) << std::endl; // If there is no command line arguments it will check the files in your MyDocument\StarCraft II and get default settings.
        std::cout<<"Executable path: " <<coordinator.GetExePath()<<std::endl;
        coordinator.SetRealtime(false);
        //coordinator.SetRealtime(true);
        coordinator.SetStepSize(step_size); // ������Ϸѭ������
        coordinator.SetMultithreaded(true);

        no_action na_bot;
        Bot bot; // traditional rule-based bot
        random_bot bot2, bot3;
        human_control bot0;
        one_frame_bot of_bot, of_bot2;
        potential_field_bot pf_bot(step_size);
        advanced_potential_field_bot adv_pf_bot(step_size);
        step_size_test_bot sstb;
        debug_test_bot dtb;
        remote_draw rdb;

        attack_nearest an_bot;

        coordinator.SetParticipants({ // ��ʼ����Ӫô
            //CreateComputer(Race::Terran),
            //CreateParticipant(Race::Terran, &na_bot), // a bot without any actions
            //CreateParticipant(Race::Terran, &bot0), // �������
            //CreateParticipant(Race::Terran, &of_bot), // one frame bot
            //CreateParticipant(Race::Terran, &bot3), // random bot
            //CreateParticipant(Race::Terran, &pf_bot), //potential field bot
            //CreateParticipant(Race::Terran, &adv_pf_bot), //advanced potential field bot
            //CreateParticipant(Race::Terran, &sstb),
            CreateParticipant(Race::Terran, &rdb), //! This is a nightmare for all units in all maps, just start screaming!

            //CreateParticipant(Race::Terran, &an_bot), // ������壬ʹ��Attack Nearest

            CreateComputer(Race::Terran), // ��ӵ��ԣ�Ĭ��easy�Ѷ�
            //CreateParticipant(Race::Terran, &bot2),
            }); // ��Ӳ������

        //coordinator.LaunchStarcraft();

        //! made it
        coordinator.Connect("59.71.231.175",3000);
        //coordinator.Connect("127.0.0.1", 8167);


        //? I need another coordinator to test
        //Coordinator coordinator2;
        //std::cout << "isLoadSettings: " << coordinator2.LoadSettings(argc, argv) << std::endl; // If there is no command line arguments it will check the files in your MyDocument\StarCraft II and get default settings.
        //std::cout << "Executable path: " << coordinator2.GetExePath() << std::endl;
        //coordinator2.SetRealtime(false);
        ////coordinator.SetRealtime(true);
        //coordinator2.SetStepSize(step_size); // ������Ϸѭ������
        //coordinator2.SetMultithreaded(true);
        //coordinator2.SetParticipants({ // ��ʼ����Ӫô
        //    //CreateParticipant(Race::Terran, &of_bot), // one frame bot
        //    CreateParticipant(Race::Terran, &an_bot), // ������壬ʹ��Attack Nearest
        //    CreateComputer(Race::Terran)
        //    }); // ��Ӳ������
        //coordinator2.Connect(8168);

        //coordinator.StartGame(sc2::kMapBelShirVestigeLE); // ��׼�Ծֵ�ͼ
        //coordinator.StartGame("..\\maps\\Test\\testBattle.SC2Map");
        //coordinator.StartGame("..\\maps\\Test\\testBattle_2distant_vs_1melee.SC2Map");
        //coordinator.StartGame("..\\maps\\Test\\testBattle_distant_vs_melee_debug.SC2Map");
        //coordinator.StartGame("..\\maps\\Test\\testBattle_distant_vs_distant.SC2Map");
        //coordinator2.StartGame("..\\maps\\Test\\testBattle_distant_vs_distant.SC2Map");
        //coordinator.StartGame("..\\maps\\Test\\testBattle_no_enemy.SC2Map");
        //coordinator.StartGame("..\\maps\\Test\\testBattleAllUnits.SC2Map");
        //coordinator.StartGame("..\\maps\\Test\\testBattle_d_m_vs_d_m.SC2Map");
        //coordinator.StartGame("..\\maps\\Test\\testBattle_m_vs_d.SC2Map");
        //coordinator.StartGame("..\\maps\\Test\\testBattle1v1.SC2Map"); // 1v1��ֹ����
        //coordinator.StartGame("..\\maps\\Test\\testMechanism_StepSize.SC2Map");

        //! if the remote client was used, the path should be set properly
        coordinator.StartGame("..\\Maps\\testBattle_distant_vs_melee_debug.SC2Map");


        /*bool is_start = false;
        time_t start = 0;
        int frames = 0;*/
        while (coordinator.Update()) { // run a bot
        //while (coordinator.Update() && coordinator2.Update()) { // run bots in two coordinators.
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