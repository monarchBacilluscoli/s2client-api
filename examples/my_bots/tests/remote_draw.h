//! This is a bot for the test of the remote headless StarCraft client's display.
//! It will use SDL lib to implement this goal.

#ifndef REMOTE_DRAW_H
#define REMOTE_DRAW_H

#include <sc2api/sc2_api.h>
#include "sc2utils/sc2_manage_process.h"

#include<iostream>
#include "../my_bots/rule_based_bots/rule_based_bot.h"
#include "sc2renderer/sc2_renderer.h"

namespace sc2 {
    class remote_draw : public Agent,public rule_based_bot
    {
    public:
        remote_draw() = default;
        ~remote_draw() = default;

        virtual void OnGameStart() final {
            //Initialize data
            m_alive_enemy_units = Agent::Observation()->GetUnits(Unit::Alliance::Enemy);

            sc2::renderer::Initialize("Feature layers", 50, 50, 2 * kDrawSize, 2 * kDrawSize);
        }

        virtual void OnStep() final {
            //Update data
            m_alive_enemy_units = Agent::Observation()->GetUnits(Unit::Alliance::Enemy);

            const SC2APIProtocol::Observation* observation = Agent::Observation()->GetRawObservation();

            const SC2APIProtocol::FeatureLayers& m = observation->feature_layer_data().renders();
            DrawFeatureLayerUnits8BPP(m.unit_density(), 0, 0);
            DrawFeatureLayer1BPP(m.selected(), kDrawSize, 0);

            const SC2APIProtocol::FeatureLayersMinimap& mi = observation->feature_layer_data().minimap_renders();
            DrawFeatureLayerHeightMap8BPP(mi.height_map(), 0, kDrawSize);
            DrawFeatureLayer1BPP(mi.camera(), kDrawSize, kDrawSize);

            sc2::renderer::Render();
        }

        virtual void OnUnitIdle(const Unit* unit) {
            const Unit* target = select_nearest_unit_from_point(unit->pos, m_alive_enemy_units);
            Agent::Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, target);
        }


    private:
        void DrawFeatureLayer1BPP(const SC2APIProtocol::ImageData& image_data, int off_x, int off_y) {
            assert(image_data.bits_per_pixel() == 1);
            int width = image_data.size().x();
            int height = image_data.size().y();
            sc2::renderer::Matrix1BPP(image_data.data().c_str(), width, height, off_x, off_y, kPixelDrawSize, kPixelDrawSize);
        }

        void DrawFeatureLayerUnits8BPP(const SC2APIProtocol::ImageData & image_data, int off_x, int off_y) {
            assert(image_data.bits_per_pixel() == 8);
            int width = image_data.size().x();
            int height = image_data.size().y();
            sc2::renderer::Matrix8BPPPlayers(image_data.data().c_str(), width, height, off_x, off_y, kPixelDrawSize, kPixelDrawSize);
        }

        void DrawFeatureLayerHeightMap8BPP(const SC2APIProtocol::ImageData & image_data, int off_x, int off_y) {
            assert(image_data.bits_per_pixel() == 8);
            int width = image_data.size().x();
            int height = image_data.size().y();
            sc2::renderer::Matrix8BPPHeightMap(image_data.data().c_str(), width, height, off_x, off_y, kPixelDrawSize, kPixelDrawSize);
        }

        const float kCameraWidth = 24.0f;
        const int kFeatureLayerSize = 80;
        const int kPixelDrawSize = 5;
        const int kDrawSize = kFeatureLayerSize * kPixelDrawSize;

    };


}

#endif // !REMOTE_DRAW_H
