#ifndef SIMULATOR_TEST_BOT
#define SIMULATOR_TEST_BOT

#include<sc2api/sc2_api.h>
#include"../RollingBot/simulator.h"
#include "sc2renderer/sc2_renderer.h"
#include "../my_bots/tests/remote_draw.h"

namespace sc2 {

    class SimulatorTestBot : public Agent
    {
    public:
        virtual void OnGameStart() final {
            sc2::FeatureLayerSettings settings(kCameraWidth, kFeatureLayerSize, kFeatureLayerSize, kFeatureLayerSize, kFeatureLayerSize);
            m_sim.SetFeatureLayers(settings);
            renderer::Initialize("Feature layers", 50, 50, 2 * kDrawSize, 2 * kDrawSize);
            m_sim.Initialize("59.71.231.175", 3000, "..\\Maps\\testBattle_distant_vs_melee_debug.SC2Map", 10, m_rdt);
            m_sim.CopyAndSetState(Observation());
            m_sim.Run(10);
            std::cout << m_sim.Observation()->GetUnits().size() << std::endl;
            m_sim.Load();
            m_sim.Run(10);
        }
    private:
        Simulator m_sim;
        remote_draw m_rdt;
        const float kCameraWidth = 24.0f;
        const int kFeatureLayerSize = 80;
        const int kPixelDrawSize = 5;
        const int kDrawSize = kFeatureLayerSize * kPixelDrawSize;

    };
}


#endif // !SIMULATOR_TEST_BOT


