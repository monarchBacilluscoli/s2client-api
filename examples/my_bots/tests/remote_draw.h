//! This is a bot for the test of the remote headless StarCraft client's display.
//! It will use SDL lib to implement this goal.

#ifndef REMOTE_DRAW_H
#define REMOTE_DRAW_H

#include <sc2api/sc2_api.h>
#include "sc2utils/sc2_manage_process.h"

#include<iostream>
//#include "../my_bots/rule_based_bots/rule_based_bot.h"
#include "sc2renderer/sc2_renderer.h"



class remote_draw : public sc2::Agent
{
public:
    remote_draw() = default;
    ~remote_draw() = default;

    virtual void OnGameStart() final {
        sc2::renderer::Initialize("Feature layers", 50, 50, 2 * kDrawSize, 2 * kDrawSize);
    }

    virtual void OnStep() final {
        ////! if you want to handle the conflict of multiple-inheritance, you can specify the class before a function call
        //const SC2APIProtocol::Observation* observation = Agent::Observation()->GetRawObservation();

        //const SC2APIProtocol::FeatureLayers& m = observation->feature_layer_data().renders();
        ///*DrawFeatureLayerUnits8BPP(m.unit_density(), 0, 0);*/

        //renderer::Render();
    }


private:
    //void DrawFeatureLayerUnits8BPP(const SC2APIProtocol::ImageData& image_data, int off_x, int off_y) {
    //    assert(image_data.bits_per_pixel() == 8);
    //    int width = image_data.size().x();
    //    int height = image_data.size().y();
    //    renderer::Matrix8BPPPlayers(image_data.data().c_str(), width, height, off_x, off_y, kPixelDrawSize, kPixelDrawSize);
    //}

    //const float kCameraWidth = 24.0f;
    const int kFeatureLayerSize = 80;
    const int kPixelDrawSize = 5;
    const int kDrawSize = kFeatureLayerSize * kPixelDrawSize;

};



#endif // !REMOTE_DRAW_H
