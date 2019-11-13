#include "sc2api/sc2_api.h"
#include "sc2lib/sc2_utils.h"
#include <numeric>

namespace sc2 {

// frames passed per game second
const int frames_per_second = 16;

Point2D FindRandomLocation(const Point2D& min, const Point2D& max) {
    assert(min.x < max.x && min.y < max.y);
    Point2D target_pos;
    float playable_w = max.x - min.x;
    float playable_h = max.y - min.y;
    target_pos.x = playable_w * GetRandomFraction() + min.x;
    target_pos.y = playable_h * GetRandomFraction() + min.y;
    return target_pos;
}

Point2D FindRandomLocation(const GameInfo& game_info) {
    return FindRandomLocation(game_info.playable_min, game_info.playable_max);
}

Point2D FindCenterOfMap(const GameInfo& game_info) {
    Point2D target_pos;
    target_pos.x = game_info.playable_max.x / 2.0f;
    target_pos.y = game_info.playable_max.y / 2.0f;
    return target_pos;
}

const Unit* SelectNearestUnitFromPoint(const Point2D& p, const Units& us) {
    float min_distance = std::numeric_limits<float>::max();
    const Unit *selected_unit = nullptr;
    float dis;
    for (const auto u : us) {
        dis = Distance2D(p, u->pos);
        if (dis < min_distance) {
            selected_unit = u;
            min_distance = dis;
        }
    }
    return selected_unit;
}

float MoveDistance(const Unit* u, int frames, const UnitTypes& uts) {
    return uts[u->unit_type].movement_speed / frames_per_second * frames;
}

// move the invalid point2D into playable area
Point2D FixOutsidePointIntoMap(const Point2D& pos, const Point2D& min, const Point2D& max){
    Point2D ret = pos;
    // check all the directions and fix them

    if(ret.x<min.x){
        ret.x = min.x+4.f;
    }else if(ret.x>max.x){
        ret.x = max.x-4.f;
    }

    if(ret.y<min.y){
        ret.y = min.y+4.f;
    }else if(ret.y>max.y){
        ret.y = max.y-4.f;
    }
    return ret;
}



}