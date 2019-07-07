#ifndef LIU_RENDERER_H
#define LIU_RENDERER_H

#include<string>
#include<vector>
#include<SDL2/SDL.h>
#include<sc2api/sc2_api.h>
#include<SDL2/SDL_ttf.h>
#include<SDL2/SDL_image.h>

namespace sc2
{
    class LiuRenderer
    {
    private:
        SDL_Window* m_window = nullptr;
        SDL_Renderer* m_renderer = nullptr;
        TTF_Font* font = nullptr;
    private:
        const int m_facing_line_length = 50;
    private:
        void DrawObservation(const ObservationInterface* observation, int offset_x, int offset_y, int w, int h);
        // according to properties of units and windows, draw the unit 
        void DrawUnit(const Unit* unit);
    public:
        LiuRenderer();
        LiuRenderer(const std::string& window_name);
        ~LiuRenderer();

        //! Draw mutilple Observations in one window
        void DrawObservations(const std::vector<const ObservationInterface*> observations);
        //! Simple method to draw one Observation in one window
        void DrawObservation(const ObservationInterface* observation);

        void SetIsDisplay(bool is_display);
    };
}

#endif //LIU_RENDERER_H
