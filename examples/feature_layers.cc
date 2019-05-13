#include "sc2api/sc2_api.h"
#include "sc2utils/sc2_manage_process.h"

#include <iostream>

#include "sc2renderer/sc2_renderer.h"

// 
const float kCameraWidth = 24.0f;
const int kFeatureLayerSize = 80;
const int kPixelDrawSize = 5;
const int kDrawSize = kFeatureLayerSize * kPixelDrawSize;

void DrawFeatureLayer1BPP(const SC2APIProtocol::ImageData& image_data, int off_x, int off_y) {
    assert(image_data.bits_per_pixel() == 1); //? Liu: huh...there is a property represent its data bits
    int width = image_data.size().x();
    int height = image_data.size().y();
    sc2::renderer::Matrix1BPP(image_data.data().c_str(), width, height, off_x, off_y, kPixelDrawSize, kPixelDrawSize);
}

void DrawFeatureLayerUnits8BPP(const SC2APIProtocol::ImageData& image_data, int off_x, int off_y) {
    assert(image_data.bits_per_pixel() == 8); //? Liu: check whether or not the data is 8 bits in depth
    //? Liu: the width and heigth are both members of ImageData.size(Size2DI), it provides the boundary of it
    int width = image_data.size().x(); 
    int height = image_data.size().y();
    //? Liu: Since we have all the properties, we can deliver the data and those properties to renderer function and let it handle them
    //? Liu: Note that the last four params are for output setting.
    sc2::renderer::Matrix8BPPPlayers(image_data.data().c_str(), width, height, off_x, off_y, kPixelDrawSize, kPixelDrawSize);
}

void DrawFeatureLayerHeightMap8BPP(const SC2APIProtocol::ImageData& image_data, int off_x, int off_y) {
    assert(image_data.bits_per_pixel() == 8);
    int width = image_data.size().x();
    int height = image_data.size().y();
    sc2::renderer::Matrix8BPPHeightMap(image_data.data().c_str(), width, height, off_x, off_y, kPixelDrawSize, kPixelDrawSize);
}

class RenderAgent : public sc2::Agent {
public:
    virtual void OnGameStart() final {
        //? Liu: this is the renderer(SDL) initialization, not the feature layer's.
        //? Liu: feature layer's was initialized in Coordinator.SetFeatureLayers()
        sc2::renderer::Initialize("Feature layers", 50, 50, 2 * kDrawSize, 2 * kDrawSize);
    }

    virtual void OnStep() final {
        const SC2APIProtocol::Observation* observation = Observation()->GetRawObservation();

        //? Liu: this is used for drawing main camera
        //? Liu: (Observation)->(ObservationFeatureLayer)->(FeatureLayers)
        const SC2APIProtocol::FeatureLayers& m = observation->feature_layer_data().renders();
        //? Liu: you can see from the spatial.proto that those data has been defined into int32/unit8/1-bit these 3 types.
        //? Liu: they are all ImageData
        //? Liu: there is a mistake, they passed a unit_density layer to a function which is used to display the aliance of units. What a big mistake
        DrawFeatureLayerUnits8BPP(m.player_relative(), 0, 0);
        //? Liu: 
        DrawFeatureLayer1BPP(m.selected(), kDrawSize, 0);

        //? Liu: (Observation)->(ObservationFeatureLayer)->(FeatureLayersMinimap)
        const SC2APIProtocol::FeatureLayersMinimap& mi = observation->feature_layer_data().minimap_renders();
        DrawFeatureLayerHeightMap8BPP(mi.height_map(), 0, kDrawSize);
        DrawFeatureLayer1BPP(mi.camera(), kDrawSize, kDrawSize);

        sc2::renderer::Render();
    }

    virtual void OnGameEnd() final {
        sc2::renderer::Shutdown();
    }
};

int main(int argc, char* argv[]) {
    sc2::Coordinator coordinator;
    if (!coordinator.LoadSettings(argc, argv)) {
        return 1;
    }

    //? Liu: here it has defined the feature layer's properties like size, etc. 
    sc2::FeatureLayerSettings settings(kCameraWidth, kFeatureLayerSize, kFeatureLayerSize, kFeatureLayerSize, kFeatureLayerSize);
    coordinator.SetFeatureLayers(settings);

    // Add the custom bot, it will control the players.
    RenderAgent bot;

    coordinator.SetParticipants({
        CreateParticipant(sc2::Race::Terran, &bot),
        CreateComputer(sc2::Race::Zerg)
    });

    // Start the game.
    coordinator.LaunchStarcraft();
    coordinator.StartGame(sc2::kMapBelShirVestigeLE);

    //? Liu: Connect to a remote game
    //coordinator.Connect("59.71.231.175", 3000);
    //coordinator.StartGame("..\\Maps\\testBattle_distant_vs_melee_debug.SC2Map");

    while (coordinator.Update()) {
        if (sc2::PollKeyPress()) {
            break;
        }
    }

    return 0;
}
