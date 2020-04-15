#include "sc2api/sc2_api.h"
#include "sc2lib/sc2_utils.h"
#include "sc2api/sc2_score.h"
#include <numeric>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>

namespace sc2
{

// frames passed per game second
const int frames_per_second = 16;

Point2D FindRandomLocation(const Point2D &min, const Point2D &max)
{
    assert(min.x < max.x && min.y < max.y);
    Point2D target_pos;
    float playable_w = max.x - min.x;
    float playable_h = max.y - min.y;
    target_pos.x = playable_w * GetRandomFraction() + min.x;
    target_pos.y = playable_h * GetRandomFraction() + min.y;
    return target_pos;
}

Point2D FindRandomLocation(const GameInfo &game_info)
{
    return FindRandomLocation(game_info.playable_min, game_info.playable_max);
}

Point2D FindCenterOfMap(const GameInfo &game_info)
{
    Point2D target_pos;
    target_pos.x = game_info.playable_max.x / 2.0f;
    target_pos.y = game_info.playable_max.y / 2.0f;
    return target_pos;
}

const Point2D &FindNearestPointFromPoint(const Point2D &p, std::vector<Point2D> ps)
{
    return *(std::min_element(ps.begin(), ps.end(), [&p](const Point2D &a, const Point2D &b) -> bool { return Distance2D(a, p) < Distance2D(b, p); }));
}

const Unit *FindNearestUnitFromPoint(const Point2D &p, const Units &us)
{
    float min_distance = std::numeric_limits<float>::max();
    const Unit *selected_unit = nullptr;
    float dis;
    for (const auto u : us)
    {
        dis = Distance2D(p, u->pos);
        if (dis < min_distance)
        {
            selected_unit = u;
            min_distance = dis;
        }
    }
    return selected_unit;
}

float MoveDistance(const Unit *u, int frames, const UnitTypes &uts)
{
    return uts[u->unit_type].movement_speed / frames_per_second * frames;
}

// move the invalid point2D into playable area
Point2D FixOutsidePointIntoRectangle(const Point2D &pos, const Point2D &min, const Point2D &max)
{
    assert(min.x < max.x && min.y < max.y);
    Point2D ret = pos;
    // check all the directions and fix them
    if (ret.x < min.x)
    {
        ret.x = min.x + 4.f;
    }
    else if (ret.x > max.x)
    {
        ret.x = max.x - 4.f;
    }

    if (ret.y < min.y)
    {
        ret.y = min.y + 4.f;
    }
    else if (ret.y > max.y)
    {
        ret.y = max.y - 4.f;
    }
    return ret;
}

Point2D FixOutsidePointIntoCircle(const Point2D &pos, const Point2D &center, float radius)
{
    assert(radius > 0.f);
    float dis = (pos - center).modulus();
    if (dis < radius)
    {
        return pos;
    }
    return (pos - center) / dis * radius + center;
}

Point2D FindNearestPointOnALine(const Point2D &point, const Point2D &line_end1, const Point2D &line_end2)
{
    Point2D nearest_point;

    return nearest_point;
}

float AreaTriangle(const Point2D &p1, const Point2D &p2, const Point2D &p3)
{
    return std::abs((p2.x * p1.y - p1.x * p2.y + p3.x * p2.y - p2.x * p3.y - p3.x * p1.y + p1.x * p3.y) / 2);
}

Point2D CalcPointOnLineByRatio(const Point2D &end1, const Point2D &end2, float ratio)
{
    Point2D vec = end2 - end1;
    return end1 + ratio * vec;
}

float GetTotalHealth(const Units &us)
{
    float total_health = std::accumulate(us.begin(), us.end(), 0.f, [](float ini_value, const Unit *u) -> float {
        return ini_value + (u->is_alive ? u->health : 0.f);
    });
    return total_health;
}

float GetTotalShield(const Units &us)
{
    float total_shield = std::accumulate(us.begin(), us.end(), 0.f, [](float ini_value, const Unit *u) -> float {
        return ini_value + u->is_alive ? u->shield : 0.f;
    });
    return total_shield;
}

float GetTotalHealthMax(const Units &us)
{
    float total_health_max = std::accumulate(us.begin(), us.end(), 0.f, [](float ini_value, const Unit *u) -> float {
        return ini_value + u->health_max;
    });
    return total_health_max;
}

float GetTotalShieldMax(const Units &us)
{
    float total_shield_max = std::accumulate(us.begin(), us.end(), 0.f, [](float ini_value, const Unit *u) -> float {
        return ini_value + u->shield_max;
    });
    return total_shield_max;
}

float GetTotalHealthLoss(const Units &us)
{
    float total_health_loss = GetTotalHealthMax(us) - GetTotalHealth(us);
    return total_health_loss;
}

float GetTotalShieldLoss(const Units &us)
{
    float total_shield_loss = GetTotalShieldMax(us) - GetTotalShield(us);
    return total_shield_loss;
}

time_t OutputGameResult(const ObservationInterface *ob, const std::string &file_path, const std::string &remark)
{
    std::fstream file;
    file.open(file_path, std::ios::app | std::ios::out); // open or create a file then append data
    if (!file.is_open())
    {
//todo check if the folder is existing, if not, create it, Attention: different system may need different method to do it
#ifdef _WIN32

#elif __linux__

#endif
        std::cerr << "failed to open or create the file! Check if the folder is existing." << std::endl;
        throw("failed to open the file!@" + std::string(__FUNCTION__));
        return time_t(0);
    }
    else
    {
        //todo maybe I need to write a data header if the file is created just now
        std::streampos fp = file.tellp();
        if (static_cast<int>(fp) == 0)
        {
            file << "total_damage_dealt.life" << '\t' << "total_damage_dealt.shields" << '\t' << "total_damage_taken.life" << '\t' << "total_damage_taken.shields" << '\t' << "total_healed.life" << '\t' << "total_healed.shields" << '\t' << "game loop" << '\t' << "map name" << '\t' << "record time" << '\t' << "remark" << std::endl;
        }
        time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        const ScoreDetails &score_details = ob->GetScore().score_details;
        std::string map_name = ob->GetGameInfo().map_name;
        file << score_details.total_damage_dealt.life << '\t' << score_details.total_damage_dealt.shields << '\t' << score_details.total_damage_taken.life << '\t' << score_details.total_damage_taken.shields << '\t' << score_details.total_healed.life << '\t' << score_details.total_healed.shields << '\t' << ob->GetGameLoop() << '\t' << map_name << '\t' << std::put_time(gmtime(&tt), "%F %T") << '\t' << remark << std::endl;
        return tt;
    }
}

void KillProcess(const std::string &string_contained)
{
    std::string kill_command = "ps -aux| grep " + string_contained + " | grep -v grep |awk '{print $2}' | xargs kill";
    system(kill_command.c_str());
}

} // namespace sc2