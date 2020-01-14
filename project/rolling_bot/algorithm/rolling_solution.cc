#include "rolling_solution.h"

namespace sc2
{
std::map<Tag, UnitStatisticalData> AverageSimData::CalculateAverUnitStatistics(const std::vector<SimData> &results)
{
    std::map<Tag, UnitStatisticalData> aver_result;
    if (results.size() == 0)
    {
        throw(std::string{"no results recorded@"} + __FUNCTION__);
    }
    else if (results.size() == 1)
    {
        for (const auto &u : results.front().units)
        {
            aver_result[u.first] = u.second.statistics;
        }
    }
    else
    {
        for (const auto &u : results.front().units) // according to the first results to traverse the units
        {
            aver_result[u.first].action_number = std::accumulate(results.begin(), results.end(), 0, [&u](int sum, const SimData &data) -> int { return sum + data.units.at(u.first).statistics.action_number; }) / results.size();
            aver_result[u.first].attack_number = std::accumulate(results.begin(), results.end(), 0, [&u](int sum, const SimData &data) -> int { return sum + data.units.at(u.first).statistics.attack_number; }) / results.size();
            aver_result[u.first].health_change = std::accumulate(results.begin(), results.end(), 0, [&u](float sum, const SimData &data) -> float { return sum + data.units.at(u.first).statistics.health_change; }) / results.size();
        }
    }
    return aver_result;
}

float AverageSimData::CalculateWinRate(const std::vector<SimData> &results)
{
    if (results.size() == 0)
    {
        throw(std::string{"no results recorded@"} + __FUNCTION__);
    }
    else if (results.size() == 1)
    {
        return results.front().game.result == GameResult::Win ? 1.f : results.front().game.result == GameResult::Tie ? 0.5f : 0.f;
    }
    else
    {
        return std::accumulate(results.begin(), results.end(), 0.f, [](float win_count, const SimData &data) -> float {
                   return win_count + data.game.result == GameResult::Win ? 1.f : data.game.result == GameResult::Tie ? 0.5f : 0.f;
               }) /
               static_cast<float>(results.size());
    }
}

} // namespace sc2