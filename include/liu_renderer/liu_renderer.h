#ifndef LIU_RENDERER_H
#define LIU_RENDERER_H

#include<string>
#include<vector>
#include"SDL.h"
#include<sc2api/sc2_api.h>

namespace sc2
{
    class LiuRenderer
    {
    private:
        SDL_Window* m_window = nullptr;
        SDL_Renderer* m_renderer = nullptr; 
    private:
        void DrawObservation(const ObservationInterface* observation, int offset_x, int offset_y, int w, int h);
    public:
        LiuRenderer();
        ~LiuRenderer();
        
        //todo
        void DrawObservations(const std::vector<const ObservationInterface*> observations);
        void DrawObservation(const ObservationInterface* observation);        
    };    

} // namespace sc2

#endif //LIU_RENDERER_H
