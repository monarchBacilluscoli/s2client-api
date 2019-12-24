#pragma once

#include "sc2api/sc2_common.h"
#include "sc2api/sc2_map_info.h"
#include <vector>
#include <numeric>

namespace sc2
{
const double PI = atan(1.) * 4.;

void KillProcess(const std::string &string_contained);

Point2D FindRandomLocation(const Point2D &min, const Point2D &max);
Point2D FindRandomLocation(const GameInfo &game_info);
Point2D FindCenterOfMap(const GameInfo &game_info);
const Unit *SelectNearestUnitFromPoint(const Point2D &p, const Units &us);
float MoveDistance(const Unit *u, int frames, const UnitTypes &uts);
Point2D FixOutsidePointIntoRectangle(const Point2D &pos, const Point2D &min, const Point2D &max);
Point2D FixOutsidePointIntoCircle(const Point2D &pos, const Point2D &center, float radius); // move outside points to the circumference of a circle, points inside stay still

float GetTotalHealth(const Units &us);     // return current total health of some units (only choose those who are alive)
float GetTotalShield(const Units &us);     // return current total health of some units (only choose those who are alive)
float GetTotalHealthMax(const Units &us);  // return the total max health of some units (all the units will be chosen)
float GetTotalShieldMax(const Units &us);  // return the total max shield of some units (all the units will be chosen)
float GetTotalHealthLoss(const Units &us); // return the total health loss of some units compared with their total max_health
float GetTotalShieldLoss(const Units &us); // return the total health loss of some units compared with their total max_shield

time_t OutputGameResult(const ObservationInterface *ob, const std::string &file_path, const std::string &remark = std::string()); // return the recording time

template <class T, class TAllocator, template <typename, typename> class TContainer>
std::string ContainerToStringWithSeparator(TContainer<T, TAllocator> container, char separator = '\t') /* we always have something stored in vec need printing */
{
    return std::accumulate(std::next(container.begin()), container.end(), std::to_string(container.front()), [separator](std::string s, const T &item) -> std::string {
        return std::move(s) + separator + std::to_string(item);
    });
}

} // namespace sc2