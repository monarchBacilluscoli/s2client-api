// Added by LiuYongfeng for easy test

#include <sc2utils/port_checker.h>
#include <sc2api/sc2_api.h>

#define CORS_COUNT 100

namespace sc2
{
    class ANBot : public Agent
    {
        void OnGameEnd() final;

        void OnUnitIdle(const Unit *u) final;
    };

    class Test
    {
    private:
        // PortChecker m_port_checker;

    public:
        Test(/* args */) = default;
        ~Test() = default;

        static void Update(Coordinator &cor, int frames);

        static void CSetAndStart(Coordinator &cor, int port, int argc, char *argv[], Agent *bot1, Agent *bot2, int time_out_ms = 10000);

        static void SetAndStartCors(int argc, char *argv[]);
    };
} // namespace sc2
