#include "liu_renderer/liu_renderer.h"
#include <iostream>
#include <cmath>

using namespace sc2;

//! A fixed size health bar
struct HealthBar{
    HealthBar(int x, int y, int max_value, int current_value){
        //todo maybe... 1 pixel 1 hp? about 10 pixels' width... I mean height
        frame = {x,y,max_value+2, 10};
        cover = {x+1,y+1,current_value, 8};
    }
    void SetCurrentValue(int current_value){
        cover.x = current_value;
    }
    // Don't forget to present the draw
    void Draw(SDL_Renderer* renderer){
        // store the original draw color of current renderer for restore it after draw
        Uint8 r,g,b,a;
        SDL_GetRenderDrawColor(renderer,&r,&g,&b,&a);
        // Black frame
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff/2);
        SDL_RenderDrawRect(renderer, &frame);
        // Red bar cover
        SDL_SetRenderDrawColor(renderer, 0xff,0,0,0xff/2);
        SDL_RenderDrawRect(renderer, &cover);
        SDL_RenderFillRect(renderer, &cover);
        SDL_SetRenderDrawColor(renderer, r,g,b,a);
        // draw the text of current health
        //todo set the message text        
    }
    SDL_Rect frame;
    SDL_Rect cover;
};



LiuRenderer::LiuRenderer()
{
    // Init SDL2
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        const char *error = SDL_GetError();
        std::cout << "SDL_Init() failed with error: " << error << std::endl;
        exit(1);
    }
    // I hope that everybody will run it with a monitor...
    SDL_Rect display_bound;
    if (SDL_GetDisplayBounds(0, &display_bound) != 0)
    {
        const char *error = SDL_GetError();
        std::cout << "SDL_GetDisplayBounds() failed with error: " << error << std::endl;
        exit(1);
    }
    m_window = SDL_CreateWindow("StarCraftII Observer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, display_bound.w*3/4, display_bound.h*3/4, SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);

    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
    // Enable the use of alpha tannel
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(m_renderer, 0xff, 0xff, 0xff, 0xff);
    SDL_RenderClear(m_renderer);
    SDL_RenderPresent(m_renderer);
}

LiuRenderer::LiuRenderer(const std::string& window_name){
    LiuRenderer();
    SDL_SetWindowTitle(m_window, window_name.c_str());
}

LiuRenderer::~LiuRenderer()
{
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void LiuRenderer::DrawObservations(const std::vector<const ObservationInterface*> observations){
    //todo get main window size
    int w, h;
    SDL_GetWindowSize(m_window, &w, &h);
    //todo split it into several sub windows
    //todo square root the observations size
    int cuts = ceil(pow(observations.size(), 0.5f));
    float w_sub = w / (float)cuts;
    float h_sub = h / (float)cuts;
    //todo draw each window
    for (size_t i = 0; i < observations.size(); i++)
    {
        //todo get the remainder to calculate coordinator x
        int offset_x = (i % cuts) * w_sub;
        //todo get the consult to calculate coordinator y
        int offset_y = (i / cuts) * h_sub;
        DrawObservation(observations[i], offset_x, offset_y, w_sub, h_sub);
    }
}


void LiuRenderer::DrawObservation(const ObservationInterface *observation)
{
    int w, h;
    SDL_GetWindowSize(m_window, &w, &h);

    SDL_SetRenderDrawColor(m_renderer, 0xff, 0xff, 0xff, 0xff);
	SDL_RenderClear(m_renderer);
    DrawObservation(observation, 0,0,w,h);
    SDL_RenderPresent(m_renderer);
}

// Without presenting and clearing
void LiuRenderer::DrawObservation(const ObservationInterface *observation, int offset_x, int offset_y, int w, int h)
{
    //calculate the ratio from original map to draw window
    Point2D playable_min = observation->GetGameInfo().playable_min;
    Vector2D playable_length = {observation->GetGameInfo().playable_max.x - playable_min.x, observation->GetGameInfo().playable_max.y - playable_min.y};
    float ratio = playable_length.x / w > playable_length.y / h ? w / playable_length.x : h / playable_length.y;
    // handle how to draw the unit
    Units us = observation->GetUnits();
    Point2D playable_pos;
    SDL_Rect unit_rect; //? Note that the properties of rect are ints
    for (const Unit *u : us)
    {
        if (u->owner == 1)
        {
            SDL_SetRenderDrawColor(m_renderer, 0, 0xff, 0, 0xff);
        }
        else if (u->owner == 2)
        {
            SDL_SetRenderDrawColor(m_renderer, 0xff, 0, 0, 0xff);
        }
        else
        {
            SDL_SetRenderDrawColor(m_renderer, 0, 0, 0xff, 0xff);
        }
        playable_pos = u->pos - playable_min;
        // get the corresponding pos in draw window
        float x = (float)offset_x + playable_pos.x * ratio;
        float y = (float)offset_y + (playable_length.y - playable_pos.y) * ratio;
        float size = u->radius * ratio < 20 ? 20 : u->radius * ratio;
        unit_rect = {(int)x, (int)y, (int)size, (int)size};
        HealthBar health_bar(x,y-12,u->health_max, u->health);
        health_bar.Draw(m_renderer);
        SDL_RenderDrawRect(m_renderer, &unit_rect);
        SDL_RenderFillRect(m_renderer, &unit_rect);
        //todo Draw the facing direction of a unit
        //todo calculate the line in the direction of the unit
        Point2DI facing_line_start(x+size/2,y+size/2); // from the middle of the unit
        SDL_SetRenderDrawColor(m_renderer,0,0,0,0xff/2);
        SDL_RenderDrawLine(m_renderer,facing_line_start.x,facing_line_start.y,facing_line_start.x+m_facing_line_length* std::cos(u->facing), facing_line_start.y-m_facing_line_length* std::sin(u->facing));
    }
}

void LiuRenderer::SetIsDisplay(bool is_display){
    if(is_display){
        SDL_ShowWindow(m_window);
    }else{
        SDL_HideWindow(m_window);
    }
}