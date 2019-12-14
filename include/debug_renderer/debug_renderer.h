#ifndef DEBUG_RENDERER_H
#define DEBUG_RENDERER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <sc2api/sc2_api.h>
#include <string>
#include <vector>
#include "../project/rolling_bot/algorithm/solution.h"
#include "../project/rolling_bot/simulator/command.h"
#include <list>

namespace sc2
{

//! SDL_Rect only contains int data, but coordinators in game map are float data
struct FloatRect
{
    float x, y;
    float w, h;
};

//! Used for transforming coordinators between game position and renderer position
class CoordinateTransformer
{
private:
    FloatRect m_game_rect = {1, 1, 1, 1};
    SDL_Rect m_render_rect = {1, 1, 1, 1};

private:
    //some intermediate data
    float m_ratio_game_to_render = 1.f;

public:
    // several constructor
    CoordinateTransformer(const Point2D &playable_min, const Point2D &playable_max, const SDL_Rect &renderer_boundary);
    CoordinateTransformer(const ObservationInterface *observation, const SDL_Rect &renderer_boundary);
    CoordinateTransformer(const SDL_Rect &renderer_boundary); // Ok, for convinent use, you can set renderer_boundry
    CoordinateTransformer(/* args */) = delete;
    ~CoordinateTransformer() = default;

public:
    void SetPlayableGameMapSize(const ObservationInterface *observation);
    void SetPlayableGameMapSize(const GameInfo &game_info);
    void SetPlayableGameMapSize(const Point2D &playable_min, const Point2D &playable_max);
    void SetRendererSize(int x, int y, int w, int h);

    float GetRatioGameToRender() const;

    Point2DI ToRenderPoint(const Point2D &game_point) const;
    Point2D ToGamePoint(const Point2D &render_point) const;
};

class DebugRenderer
{
private:
    // some objects;
    SDL_Window *m_window = nullptr;
    SDL_Renderer *m_renderer = nullptr;
    TTF_Font *font = nullptr;

private:
    // some settings
    int m_facing_line_length = 50;
    static float ratio_between_window_and_unit;

private:
    void DrawObservation(const ObservationInterface *observation, int offset_x, int offset_y, int w, int h);
    void DrawOrders(const std::vector<Command> &orders, const ObservationInterface *observation, const std::map<Tag, const Unit *> &units_map, int x, int y, int w, int h);
    void DrawOrders(const std::vector<Command> &orders, const ObservationInterface *observation, int x, int y, int w, int h);
    // according to properties of units and windows, draw the unit
    void DrawUnit(const Unit *unit);

private:
    static void DrawCenterRect(SDL_Renderer *renderer, const SDL_Rect *rect);

public:
    DebugRenderer();
    DebugRenderer &operator=(const sc2::DebugRenderer &rhs);
    // todo finish it
    DebugRenderer(const std::string &window_name, int offset_x, int offset_y, int w, int h);
    DebugRenderer(const std::string &window_name);
    ~DebugRenderer();

    void Reconstruct();

    void ClearRenderer();

    void DrawSolution(const Solution<Command> &solution, const ObservationInterface *observation, const std::map<Tag, const Unit *> &units_map);
    void DrawOrders(const std::vector<Command> &orders, const ObservationInterface *observation, const std::map<Tag, const Unit *> &units_map);
    void DrawOrders(const std::vector<Command> &orders, const ObservationInterface *observation);

    //! Draw mutilple Observations in one window
    void DrawObservations(const std::vector<const ObservationInterface *> observations);
    //! Simple method to draw one Observation in one window
    void DrawObservation(const ObservationInterface *observation);

    void SetIsDisplay(bool is_display);

    //! for test
    void DrawRedRect()
    {
        SDL_SetRenderDrawColor(m_renderer, 0xff, 0xff, 0xff, 0xff);
        SDL_RenderClear(m_renderer);
        SDL_Rect rect = {0, 0, 50, 50};
        SDL_SetRenderDrawColor(m_renderer, 0xff, 0, 0, 0xff);
        SDL_RenderDrawRect(m_renderer, &rect);
        SDL_RenderPresent(m_renderer);
    }

    void Present();
};

class DebugRenderers
{
private:
    std::vector<DebugRenderer> m_debug_renderers;

public:
    DebugRenderers() : DebugRenderers(0){};
    //! according to the monitor windows size, set the subwindows size and positions
    DebugRenderers(int count);
    // I dont have to write the destructor, since program will call the item's destructor automatically when they are no longer useful

    // Reconstruct all renderers
    void ReconstructAll();
    // get size
    size_t size() { return m_debug_renderers.size(); };
    // get subwindow
    DebugRenderer &operator[](int count);
};
} // namespace sc2

struct CandidateColor
{
    Uint8 r, g, b;
    Uint8 a;
};

#endif //DEBUG_RENDERER_H
