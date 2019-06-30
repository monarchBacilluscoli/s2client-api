#pragma once

#include "sc2api/sc2_common.h"
#include "sc2api/sc2_map_info.h"

namespace sc2 {

Point2D FindRandomLocation(const Point2D& min, const Point2D& max);
Point2D FindRandomLocation(const GameInfo& game_info);
Point2D FindCenterOfMap(const GameInfo& game_info);

const Unit *SelectNearestUnitFromPoint(const Point2D &p, const Units &us);
float MoveDistance(const Unit* u, int frames, const UnitTypes& uts);
}  // namespace sc2