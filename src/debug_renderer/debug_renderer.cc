#include "debug_renderer/debug_renderer.h"
#include <iostream>
#include <cmath>
#include <thread>

using namespace sc2;

float DebugRenderer::ratio_between_window_and_unit = 20.f;

CoordinateTransformer::CoordinateTransformer(const ObservationInterface* observation, const SDL_Rect& renderer_boundary) : CoordinateTransformer(observation->GetGameInfo().playable_min, observation->GetGameInfo().playable_max, renderer_boundary) {}

CoordinateTransformer::CoordinateTransformer(const Point2D& playable_min, const Point2D& playable_max, const SDL_Rect& renderer_boundary):
m_game_rect({playable_min.x, playable_min.y,(playable_max-playable_min).x,(playable_max-playable_min).y}),
m_render_rect(renderer_boundary),
m_ratio_game_to_render(m_game_rect.w/m_render_rect.w>m_game_rect.h/m_render_rect.h?m_render_rect.w/m_game_rect.w:m_render_rect.h/m_game_rect.h)
{}

Point2DI CoordinateTransformer::ToRenderPoint(const Point2D& game_point) const {
    Point2D pos_relative_playable_min = game_point - Point2D(m_game_rect.x, m_game_rect.y);
    return Point2DI(
        pos_relative_playable_min.x * m_ratio_game_to_render + m_render_rect.x,
        (m_game_rect.h - pos_relative_playable_min.y) * m_ratio_game_to_render + m_render_rect.y);
}

Point2D CoordinateTransformer::ToGamePoint(const Point2D& render_point) const {
    Point2D pos_relative_render_min = render_point - Point2D(m_render_rect.x, m_render_rect.y);
    return Point2D(pos_relative_render_min.x / m_ratio_game_to_render + m_game_rect.x, m_game_rect.h - pos_relative_render_min.y / m_ratio_game_to_render + m_game_rect.y);
}

void CoordinateTransformer::SetPlayableGameMapSize(const ObservationInterface* observation){
    SetPlayableGameMapSize(observation->GetGameInfo());
}

void CoordinateTransformer::SetPlayableGameMapSize(const GameInfo& game_info){
    SetPlayableGameMapSize(game_info.playable_min, game_info.playable_max);
}

void CoordinateTransformer::SetPlayableGameMapSize(const Point2D& playable_min, const Point2D& playable_max){
    Vector2D playable_length = playable_max - playable_min;
    m_game_rect = {
        playable_min.x,
        playable_min.y,
        playable_length.x,
        playable_length.y};
}

float CoordinateTransformer::GetRatioGameToRender() const {
    return m_ratio_game_to_render;
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

struct CenterRect
{
    CenterRect(int x,int y,int w,int h):rect{x-w/2,y-h/2,w,h}{}
    SDL_Rect rect;
};


// void DebugRenderer::DrawCenterRect(SDL_Renderer* renderer, const SDL_Rect* rect){
//     SDL_Rect new_rect = {rect->x + rect->w / 2, rect->y + rect->h / 2, rect->w, rect->h};
//     SDL_RenderDrawRect(renderer, &new_rect);
// }

DebugRenderer::DebugRenderer() : DebugRenderer("Debug Renderer"){}

// use 3/4 of the screen size of default monitor to display
DebugRenderer::DebugRenderer(const std::string& window_name){
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

    // initialize the transformer

    SDL_SetRenderDrawColor(m_renderer, 0xff, 0xff, 0xff, 0xff);
    SDL_RenderClear(m_renderer);
    SDL_RenderPresent(m_renderer);
}

// Set your own size of debug display
DebugRenderer::DebugRenderer(const std::string& window_name, int x, int y, int w, int h){
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

void DebugRenderer::ClearRenderer() {
    SDL_SetRenderDrawColor(m_renderer, 0xff, 0xff, 0xff, 0xff);
    SDL_RenderClear(m_renderer);
}

void DebugRenderer::Present(){
    SDL_RenderPresent(m_renderer);
}

void DebugRenderer::DrawSolution(const Solution<Command>& solution, const ObservationInterface* observation, const std::map<Tag, const Unit*>& units_map){
    int w, h;
    SDL_GetWindowSize(m_window, &w, &h);
    DrawOrders(solution.variable, observation, units_map, 0, 0, w, h);
    //! for test
    // DrawRedRect();
}

void DebugRenderer::DrawOrders(const std::vector<Command>& orders, const ObservationInterface* observation, const std::map<Tag, const Unit*>& units_map){
    int w, h;
    SDL_GetWindowSize(m_window, &w, &h);
    DrawOrders(orders, observation, units_map, 0, 0, w, h);
}

void DebugRenderer::DrawOrders(const std::vector<Command>& orders, const ObservationInterface *observation, const std::map<Tag, const Unit*>& units_map, int x, int y, int w, int h){
    CoordinateTransformer transformer(observation, {x, y, w, h});
    SDL_SetRenderDrawColor(m_renderer, 0xff/2, 0xff/2, 0, 0xff/2);
    for (const Command& command : orders) {
        //todo draw each command
        {
            if(units_map.find(command.unit_tag)==units_map.end()){
                std::cout << "unit_tag:" << command.unit_tag << "\t"
                          << "units_map size: " << units_map.size() << "\t"
                          << "units size: " << observation->GetUnits().size() << std::endl;
            }
        }
        const Unit* command_unit = units_map.at(command.unit_tag);
        Point2DI start_point = transformer.ToRenderPoint(command_unit->pos);
        for (const ActionRaw& action : command.actions) {
            Point2DI end_point;
            switch (action.target_type) {
                case ActionRaw::TargetType::TargetNone: {
                    //if there is an action apply on itself, draw a small rect here
                    end_point = start_point; // It doesn't have to transform to render point
                    CenterRect self_action_position(start_point.x, start_point.y, 3, 3);
                    SDL_RenderDrawRect(m_renderer, &self_action_position.rect);
                } break;
                case ActionRaw::TargetType::TargetPosition: {
                    end_point = transformer.ToRenderPoint(action.target_point);
                    SDL_RenderDrawLine(m_renderer, start_point.x, start_point.y, end_point.x, end_point.y);

                } break;
                case ActionRaw::TargetType::TargetUnitTag: {
                    const Unit* target_u = units_map.at(action.TargetUnitTag);
                    end_point = transformer.ToRenderPoint(target_u->pos);
                    SDL_RenderDrawLine(m_renderer, start_point.x, start_point.y, end_point.x, end_point.y);
                } break;
            }
            start_point = end_point;
        }
    }
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

    DrawObservation(observation, 0,0,w,h);
}

// Without presenting and clearing
void DebugRenderer::DrawObservation(const ObservationInterface *observation, int offset_x, int offset_y, int w, int h)
{
    CoordinateTransformer coordinate_transformer(observation, {offset_x, offset_y, w, h});
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
        Point2DI render_pos = coordinate_transformer.ToRenderPoint(u->pos);
        int size = u->radius * coordinate_transformer.GetRatioGameToRender() < minimize_size ? minimize_size : ceil(u->radius * coordinate_transformer.GetRatioGameToRender());
        unit_rect = {(int)render_pos.x, (int)render_pos.y, (int)size, (int)size};
        HealthBar health_bar(render_pos.x, (int)render_pos.y - ceil(h / 30), u->health_max, u->health, ceil(h / 30), w / 240);
        health_bar.Draw(m_renderer);
        SDL_RenderDrawRect(m_renderer, &unit_rect);
        SDL_RenderFillRect(m_renderer, &unit_rect);
        //todo draw the cooldown
        if (u->weapon_cooldown > 0) {
            SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 0xff / 2);
            SDL_Rect cooldown_rect = {unit_rect.x + unit_rect.w * 1 / 4, unit_rect.y + unit_rect.h * 1 / 4, unit_rect.w / 2, unit_rect.h / 2};
            SDL_RenderDrawRect(m_renderer, &cooldown_rect);
            SDL_RenderFillRect(m_renderer, &cooldown_rect);
        }
        //todo Draw the facing direction of a unit
        //todo calculate the line in the direction of the unit
        Point2DI facing_line_start((int)render_pos.x + size / 2, (int)render_pos.y + size / 2);  // from the middle of the unit
        SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 0xff / 2);
        SDL_RenderDrawLine(m_renderer, facing_line_start.x, facing_line_start.y, facing_line_start.x + m_facing_line_length * std::cos(u->facing), facing_line_start.y - m_facing_line_length * std::sin(u->facing));
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