#pragma once

#include "sc2api/sc2_common.h"
#include "sc2api/sc2_map_info.h"

namespace sc2 {

Point2D FindRandomLocation(const Point2D& min, const Point2D& max);
Point2D FindRandomLocation(const GameInfo& game_info);
Point2D FindCenterOfMap(const GameInfo& game_info);

const Unit *select_nearest_unit_from_point(const Point2D &p, const Units &us);
} // namespace sc2d