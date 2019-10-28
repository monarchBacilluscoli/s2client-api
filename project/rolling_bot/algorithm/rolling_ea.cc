#include "rolling_ea.h"
#include <sc2lib/sc2_utils.h>

namespace sc2
{
void RollingEA::Initialize(const ObservationInterface *observation)
{
    m_observation = observation;
}

void RollingEA::InitBeforeRun()
{
    EvolutionaryAlgorithm::InitBeforeRun();
    InitFromObservation();
}

void RollingEA::Generate()
{
    for (size_t i = 0; i < m_population_size; i++)
    {
        GenerateOne(m_population[i]);
    }
}

void RollingEA::Evaluate()
{
    if (m_current_generation == 0)
    {
        Evaluate(m_population);
    }
    else
    {
        Evaluate(m_offspring);
    }
}

void RollingEA::Evaluate(Population &pop)
{
    m_simulation_pool.CopyStateAndSendOrdersAsync(m_observation, m_population);
    m_simulation_pool.RunSimsAsync(m_run_length, m_debug_renderers);

    float self_loss = 0, self_team_loss_total = 0, self_team_loss_best = std::numeric_limits<float>::max();
    float enemy_loss = 0, enemy_team_loss_total = 0, enemy_team_loss_best = std::numeric_limits<float>::lowest();
    size_t sz = m_population.size();
    for (size_t i = 0; i < sz; i++)
    {
        self_loss = m_simulation_pool.GetTeamHealthLoss(i, Unit::Alliance::Self);
        enemy_loss = m_simulation_pool.GetTeamHealthLoss(i, Unit::Alliance::Enemy);

        // set the 2 objectives
        pop[i].objectives[0] = enemy_loss;
        pop[i].objectives[1] = -self_loss;
    }
}

void RollingEA::Select()
{
    Solution<Command>::DominanceSort(m_offspring);
    std::vector<Solution<Command>> pop_pool;
    //! looks like m_population must have been sorted
    std::merge(m_population.begin(), m_population.end(), m_offspring.begin(), m_offspring.end(), std::back_inserter(pop_pool), Solution<Command>::RankLess);
    pop_pool.resize(m_population_size);
    std::swap(pop_pool, m_population);
}

void RollingEA::InitFromObservation()
{
    m_my_team = m_observation->GetUnits(Unit::Alliance::Self);
    m_game_info = m_observation->GetGameInfo();
    m_playable_dis = Vector2D(m_game_info.playable_max.x - m_game_info.playable_min.x, m_game_info.playable_max.y - m_game_info.playable_min.y);
}

void RollingEA::ShowSolutionDistribution(int showed_generations_count)
{
    int real_group_sz = std::min({showed_generations_count, ScatterRenderer2D::MaxGroupSize(), (int)(m_history_objs.size())});
    std::list<std::vector<std::vector<float>>>::iterator end = m_history_objs.end(), begin = m_history_objs.end();
    std::advance(begin, -real_group_sz); // show the last real_group_sz generations' solution distribution
    std::vector<std::string> group_names(real_group_sz);
    for (size_t i = 0; i < real_group_sz; i++)
    {
        group_names[i] = "current generation - " + std::to_string(real_group_sz - i);
    }
    m_objective_distribution.Show(begin, end, group_names);
}

void RollingEA::GenerateOne(Solution<Command> &sol)
{
    size_t team_size = m_my_team.size();
    RawActions raw_actions(m_command_length);
    for (size_t i = 0; i < team_size; i++)
    {
        sol.variable[i].unit_tag = m_my_team[i]->tag;
        // float move_dis_per_run = MoveDistance(m_my_team[i], m_run_length, m_unit_type);
        float move_dis_per_run = m_playable_dis.y / 3;
        float longest_map_bound = std::max(m_playable_dis.x, m_playable_dis.y);
        float moveable_radius = std::min(longest_map_bound, move_dis_per_run); //todo Think about the boundaries of the map!
        Point2D current_location = m_my_team[i]->pos;
        sol.variable[i].actions.resize(m_command_length);
        for (size_t j = 0; j < m_command_length; j++)
        {
            ActionRaw &action_raw = sol.variable[i].actions[j];
            // randomly choose to move or attack
            if (GetRandomFraction() < m_attack_possibility)
            {
                // randomly choose a location to attack...
                action_raw.ability_id = ABILITY_ID::ATTACK;
                action_raw.target_type = ActionRaw::TargetType::TargetPosition; //? pay attention here is my test code which need to changes
                if (j == 0)
                {
                    action_raw.target_point = current_location + Point2DP(GetRandomFraction() * moveable_radius, GetRandomFraction() * 2 * PI).toPoint2D();
                }
                else
                {
                    // hope the last target point is a move position
                    action_raw.target_point = sol.variable[i].actions[j - 1].target_point + Point2DP(GetRandomFraction() * moveable_radius, GetRandomFraction() * 2 * PI).toPoint2D();
                }
                action_raw.target_point = FixOutsidePointIntoMap(action_raw.target_point, m_game_info.playable_min, m_game_info.playable_max);
            }
            else
            {
                action_raw.ability_id = ABILITY_ID::MOVE;
                action_raw.target_type = ActionRaw::TargetType::TargetPosition;
                // construct move action
                if (j == 0)
                {
                    action_raw.target_point = current_location + Point2DP(GetRandomFraction() * moveable_radius, GetRandomFraction() * 2 * PI).toPoint2D();
                }
                else
                {
                    action_raw.target_point = sol.variable[i].actions[j - 1].target_point + Point2DP(GetRandomFraction() * moveable_radius, GetRandomFraction() * 2 * PI).toPoint2D();
                }
                action_raw.target_point = FixOutsidePointIntoMap(action_raw.target_point, m_game_info.playable_min, m_game_info.playable_max);
            }
        }
    }
}
} // namespace sc2