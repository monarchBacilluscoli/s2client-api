#include "debug_renderer/debug_renderer.h"
#include <iostream>
#include <cmath>
#include <thread>

using namespace sc2;

float DebugRenderer::ratio_between_window_and_unit = 20.f;

//! SDL_Rect only contains int data, but coordinators in game map are float data
struct FloatRect{
    float x, y;
    float w, h;
};

//! Used for transforming coordinators between game position and renderer position
class CoordinatorTransformer {
   private:
    FloatRect m_game_rect;
    SDL_Rect m_render_rect;

   private:
    //some intermediate data
    float m_ratio_game_to_render = 1.f;

   public:
    // several constructor
    CoordinatorTransformer(const Point2D& playable_min, const Point2D& playable_max, const SDL_Rect& renderer_boundary);
    CoordinatorTransformer(const ObservationInterface* observation, const SDL_Rect& renderer_boundary);
    CoordinatorTransformer(/* args */) = delete;
    ~CoordinatorTransformer() = default;

   public:
    Point2D ToRenderPoint(const Point2D& game_point) const;
    Point2D ToGamePoint(const Point2D& render_point) const;
};

CoordinatorTransformer::CoordinatorTransformer(const ObservationInterface* observation, const SDL_Rect& renderer_boundary) : CoordinatorTransformer(observation->GetGameInfo().playable_min, observation->GetGameInfo().playable_max, renderer_boundary) {}

CoordinatorTransformer::CoordinatorTransformer(const Point2D& playable_min, const Point2D& playable_max, const SDL_Rect& renderer_boundary):
m_game_rect({playable_min.x, playable_min.y,(playable_max-playable_min).x,(playable_max-playable_min).y}),
m_render_rect(renderer_boundary),
m_ratio_game_to_render(m_game_rect.w/m_render_rect.w>m_game_rect.h/m_render_rect.h?m_render_rect.w/m_game_rect.w:m_render_rect.h/m_game_rect.h)
{}

Point2D CoordinatorTransformer::ToRenderPoint(const Point2D& game_point) const {
    Point2D playable_pos = game_point - Point2D(m_game_rect.x, m_game_rect.y);
    return Point2D(
        playable_pos.x * m_ratio_game_to_render,
        (m_game_rect.y - playable_pos.y) * m_ratio_game_to_render);
}

Point2D CoordinatorTransformer::ToGamePoint(const Point2D& render_point) const {
    return Point2D(render_point.x / m_ratio_game_to_render + m_game_rect.x, m_game_rect.h - render_point.y / m_ratio_game_to_render + m_game_rect.y);
}

//! A fixed size health bar
struct HealthBar{
    HealthBar(int x, int y, int max_value, int current_value, int height = 10, float health_scale = 1.f){
        if (height > 2) { // if height>2, it means there is still space between the upper and lower lines of frames to set the cover bar. 
            frame = {x, y, ceil(max_value*health_scale) + 2, height};
            cover = {x + 1, y + 1, current_value*health_scale, height - 2};
        }else{// If height<2, it means the frame and cover must be overlaped
            frame = {x, y, ceil(max_value*health_scale), height};
            cover = {x, y, current_value*health_scale, height};
        }
    }
    void SetCurrentValue(int current_value){
        cover.x = current_value;
    }
    // Don't forget to present the draw
    void Draw(SDL_Renderer* renderer){
        // store the original draw color of current renderer for restore it after draw
        Uint8 r,g,b,a;
        SDL_GetRenderDrawColor(renderer,&r,&g,&b,&a);
        // Black frame first
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff/2);
        SDL_RenderDrawRect(renderer, &frame);
        // Red bar cover then
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

// DebugRenderer DebugRenderer::global_debug_renderer = DebugRenderer("Debug Renderer");

DebugRenderer::DebugRenderer():DebugRenderer("Debug Renderer"){}

DebugRenderer::DebugRenderer(const std::string& window_name){
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

DebugRenderer::DebugRenderer(const std::string& window_name, int x, int y, int w, int h){
// Init SDL2
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        const char *error = SDL_GetError();
        std::cout << "SDL_Init() failed with error: " << error << std::endl;
        exit(1);
    }
    m_window = SDL_CreateWindow(window_name.c_str(), x, y, w, h, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS);
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
    // Enable the use of alpha tannel
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(m_renderer, 0xff, 0xff, 0xff, 0xff);
    SDL_RenderClear(m_renderer);
    SDL_RenderPresent(m_renderer);
}

DebugRenderer::~DebugRenderer()
{
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

DebugRenderer& DebugRenderer::operator=(const sc2::DebugRenderer& rhs){
    //todo destory current pointer
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    //todo get the passed object‘s pointer
    m_renderer = rhs.m_renderer;
    m_window = rhs.m_window;
    m_facing_line_length = rhs.m_facing_line_length;

    return *this;
}

void DebugRenderer::DrawSolution(Solution<Command> solution, const ObservationInterface* observation, std::map<Tag, const Unit*> units_map){
    // todo I must be able to draw all aspects of an ActionRaw: executor, target pos, target unit, and maybe target itself
    //todo coordinator map
    //todo 
    //todo for each solution
}


void DebugRenderer::DrawObservations(const std::vector<const ObservationInterface*> observations){
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


void DebugRenderer::DrawObservation(const ObservationInterface *observation)
{
    int w, h;
    SDL_GetWindowSize(m_window, &w, &h);

    SDL_SetRenderDrawColor(m_renderer, 0xff, 0xff, 0xff, 0xff);
	SDL_RenderClear(m_renderer);
    DrawObservation(observation, 0,0,w,h);
    SDL_RenderPresent(m_renderer);
}

// Without presenting and clearing
void DebugRenderer::DrawObservation(const ObservationInterface *observation, int offset_x, int offset_y, int w, int h)
{
    //calculate the ratio from original map to draw window
    Point2D playable_min = observation->GetGameInfo().playable_min;
    Vector2D playable_length = {observation->GetGameInfo().playable_max.x - playable_min.x, observation->GetGameInfo().playable_max.y - playable_min.y};
    float ratio = playable_length.x / w > playable_length.y / h ? w / playable_length.x : h / playable_length.y;
    // handle how to draw the unit
    Units us = observation->GetUnits();
    Point2D playable_pos;
    SDL_Rect unit_rect; //? Note that the properties of rect are ints
    int minimize_size = ceil(h < w ? h / ratio_between_window_and_unit : w / ratio_between_window_and_unit);
    for (const Unit *u : us) {
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
        int size = u->radius * ratio < minimize_size ? minimize_size : ceil(u->radius * ratio);
        unit_rect = {(int)x, (int)y, (int)size, (int)size};
        HealthBar health_bar(x,y-ceil(h/30),u->health_max, u->health, ceil(h/30), w/240);
        health_bar.Draw(m_renderer);
        SDL_RenderDrawRect(m_renderer, &unit_rect);
        SDL_RenderFillRect(m_renderer, &unit_rect);
        //todo draw the cooldown
        if(u->weapon_cooldown>0){
            SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 0xff / 2);
            SDL_Rect cooldown_rect = {unit_rect.x + unit_rect.w * 1 / 4, unit_rect.y + unit_rect.h * 1 / 4, unit_rect.w / 2, unit_rect.h / 2};
            SDL_RenderDrawRect(m_renderer, &cooldown_rect);
            SDL_RenderFillRect(m_renderer, &cooldown_rect);
        }
        //todo Draw the facing direction of a unit
        //todo calculate the line in the direction of the unit
        Point2DI facing_line_start(x+size/2,y+size/2); // from the middle of the unit
        SDL_SetRenderDrawColor(m_renderer,0,0,0,0xff/2);
        SDL_RenderDrawLine(m_renderer,facing_line_start.x,facing_line_start.y,facing_line_start.x+m_facing_line_length* std::cos(u->facing), facing_line_start.y-m_facing_line_length* std::sin(u->facing));
    }
}

void DebugRenderer::SetIsDisplay(bool is_display){
    if(is_display){
        SDL_ShowWindow(m_window);
    }else{
        SDL_HideWindow(m_window);
    }
}

DebugRenderers::DebugRenderers(int count){
    m_debug_renderers.reserve(count);
    // get the monitor's size, I'd rather use the second monitor to display this
    int display_count = SDL_GetNumVideoDisplays();
    
    int chosen_display_index = display_count > 1 ? 1: 0;
    SDL_Rect display_bound;
    if (display_count < 1) {
        std::cout << "no local monitor can be detected by SDL, you may be using remote desktop, renderer will use the default settings" << std::endl;
        display_bound = {0, 0, 1920, 1080};
    } else {
        chosen_display_index = display_count > 1 ? 1: 0;
        SDL_GetDisplayBounds(chosen_display_index, &display_bound);
    }

    // split the display according to the input count
    int cuts = ceil(pow(count, 0.5f));
    float w_sub = display_bound.w / (float)cuts;
    float h_sub = display_bound.h / (float)cuts;
    //todo multi-threaded modification
    for (size_t i = 0; i < count; i++) {
        //todo get the remainder to calculate coordinator x
        int offset_x = (i % cuts) * w_sub;
        //todo get the consult to calculate coordinator y
        int offset_y = (i / cuts) * h_sub;
        m_debug_renderers.emplace_back(std::to_string(i), offset_x, offset_y, w_sub, h_sub);
    }
}

DebugRenderer& DebugRenderers::operator[] (int count){
    return m_debug_renderers[count];
}