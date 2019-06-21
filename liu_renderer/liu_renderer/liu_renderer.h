#ifndef LIU_RENDERER_H
#define LIU_RENDERER_H

#include<string>
#include<vector>
#include<SDL.h>
#include<sc2api/sc2_api.h>

namespace sc2 {

    // looks like that rect can be used to represent a small window displayed in my debug window
    //using SubWindow = SDL_Rect;

    class LiuRenderer
    {
    public:
        LiuRenderer();
        ~LiuRenderer();

        void DrawObservations(const std::vector<const ObservationInterface*> observations);
        // Draws only one Observation data to fullfill m_window
        void DrawObservation(const ObservationInterface* observation);

    private:
        void DrawObservation(const ObservationInterface* observation, int x, int y, int w, int h);

        SDL_Window* m_window = nullptr;
        SDL_Renderer* m_renderer = nullptr;
    };
}


#endif // !LIU_RENDERER_H
