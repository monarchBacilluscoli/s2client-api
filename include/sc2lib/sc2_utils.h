#pragma once

#include "sc2api/sc2_common.h"
#include "sc2api/sc2_map_info.h"

namespace sc2
{
const double PI = atan(1.) * 4.;

void KillProcess(const std::string& string_contained);

Point2D FindRandomLocation(const Point2D &min, const Point2D &max);
Point2D FindRandomLocation(const GameInfo &game_info);
Point2D FindCenterOfMap(const GameInfo &game_info);
const Unit *SelectNearestUnitFromPoint(const Point2D &p, const Units &us);
float MoveDistance(const Unit *u, int frames, const UnitTypes &uts);
Point2D FixOutsidePointIntoMap(const Point2D &pos, const Point2D &min, const Point2D &max);

float GetTotalHealth(const Units &us); // return current total health of some units (only choose those who are alive)
float GetTotalShield(const Units &us); // return current total health of some units (only choose those who are alive)
float GetTotalHealthMax(const Units &us); // return the total max health of some units (all the units will be chosen)
float GetTotalShieldMax(const Units &us); // return the total max shield of some units (all the units will be chosen)
float GetTotalHealthLoss(const Units &us); // return the total health loss of some units compared with their total max_health
float GetTotalShieldLoss(const Units &us); // return the total health loss of some units compared with their total max_shield

void OutputGameResultData(const ObservationInterface *ob, const std::string file_path);
} // namespace sc2