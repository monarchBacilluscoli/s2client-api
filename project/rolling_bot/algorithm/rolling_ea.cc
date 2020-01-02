#include "global_defines.h"

#include "rolling_ea.h"
#include <sc2lib/sc2_utils.h>

namespace sc2
{

bool RollingEA::ConvergenceTermination::operator()()
{
    std::vector<float> current_averages = m_algo.GetLastObjsAverage();
    if (m_last_record_obj_average.empty()) // the first time to check
    {
        m_last_record_obj_average = current_averages;
        return false;
    }
    float current_difference = current_averages[0] - current_averages[1];
    float last_difference = m_last_record_obj_average[0] - m_last_record_obj_average[1];
    m_last_record_obj_average = current_averages;
    if (std::abs(current_difference - last_difference) > m_no_improve_threshold) // improve from last generation
    {
        m_current_no_improve_generation = 0;
        return false;
    }
    else if (++m_current_no_improve_generation < m_max_no_impreve_generation) // no improvement from last generation
    {
        return false;
    }
    else
    {
        return true;
    }
}

void RollingEA::ConvergenceTermination::clear()
{
    m_current_no_improve_generation = 0;
    m_last_record_obj_average.clear();
}

void RollingEA::Initialize(const ObservationInterface *observation)
{
    m_observation = observation;
}

void RollingEA::InitBeforeRun()
{
    EvolutionaryAlgorithm::InitBeforeRun();
    InitOnlySelfMembersBeforeRun();
}

void RollingEA::InitOnlySelfMembersBeforeRun()
{                                              // doesn't call the base class's Init function
    InitFromObservation();                     // set the m_my_team and some other things
    m_convergence_termination_manager.clear(); // must refresh the state of it
    for (Solution<Command> &sol : m_population)
    {
        sol.variable.resize(m_my_team.size());
        sol.objectives.resize(m_objective_size);
    }
}

void RollingEA::Generate()
{
    for (size_t i = 0; i < m_population_size; i++)
    {
        GenerateOne(m_population[i]);
    }
    if (m_use_priori) // generate some solutions with priori knowledge
    {
        int enemy_sz = std::min(m_enemy_team.size(), (size_t)m_population_size / 5);
        for (size_t i = 0; i < enemy_sz; ++i)
        {
            Solution<Command> &random_sol = m_population[i];
            Point2D target_position = m_enemy_team[i]->pos;
            int unit_sz = random_sol.variable.size();
            for (size_t j = 0; j < unit_sz; ++j)
            {
                const Unit *unit = m_observation->GetUnit(random_sol.variable[j].unit_tag);
                RawActions &actions = random_sol.variable[j].actions;
                int action_sz = actions.size();
                for (size_t k = 0; k < action_sz; k++)
                {
                    Vector2D u_to_e = target_position - unit->pos;
                    actions[k].target_point = unit->pos + u_to_e * ((u_to_e.modulus() - m_unit_types[unit->unit_type].weapons.front().range) / u_to_e.modulus());
                }
            }
        }
    }
}

void RollingEA::Evaluate()
{
    //todo insert some solutions before each evaluation
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
    size_t pop_sz = pop.size();
    for (auto &item : pop)
    {
        item.results.resize(m_evaluation_time_multiplier);
        item.objectives.resize(m_objective_size);
        for (auto &ob : item.objectives)
        {
            ob = 0.f; // clear the objective value for the addition operation
        }
    }
    for (size_t j = 0; j < m_evaluation_time_multiplier; ++j)
    {
        m_simulation_pool.CopyStateAndSendOrdersAsync(m_observation, pop); // Start all simulations at the same time
        if (m_is_debug)
        {
#ifdef USE_GRAPHICS
            m_simulation_pool.RunSimsAsync(m_sim_length, m_debug_renderers);
#else
            m_simulation_pool.RunSimsAsync(m_sim_length);
#endif //USE_GRAPHICS
        }
        else
        {
            m_simulation_pool.RunSimsAsync(m_sim_length);
        }
        size_t pop_sz = pop.size();
        for (size_t i = 0; i < pop_sz; i++)
        {
            { // record game results
                const std::map<Tag, const Unit *> &units_correspondence = m_simulation_pool[i].GetRelativeUnits(); // Units final state in simulation
#ifdef DEBUG
                if (i + 1 < pop_sz && m_simulation_pool[i].GetRelativeUnits().size() != m_simulation_pool[i + 1].GetRelativeUnits().size())
                {
                    std::cout << "some mistake in return units map" << std::endl;
                }
#endif // DEBUG
                for (const auto &unit : units_correspondence)
                {
                    pop[i].results[j].units[unit.first].final_state = *(unit.second);
                    pop[i].results[j].units[unit.first].statistics = m_simulation_pool[i].GetUnitStatistics(unit.first);
                    pop[i].results[j].game.result = m_simulation_pool[i].CheckGameResult();
                }
                // pop[i].CalculateAver(); // based on the recorded statistics, calculate the average results
            }
            { // set the objectives
                pop[i].objectives[0] += m_simulation_pool.GetTeamHealthLoss(i, Unit::Alliance::Enemy);
                pop[i].objectives[1] += -m_simulation_pool.GetTeamHealthLoss(i, Unit::Alliance::Self);
            }
        }
    }
    // write the real obj values to those solutions
    for (size_t i = 0; i < pop_sz; i++)
    {
        pop[i].objectives[0] /= m_evaluation_time_multiplier;
        pop[i].objectives[1] /= m_evaluation_time_multiplier; // transform it to maximum optimization
    }
}

void RollingEA::Select()
{
    m_population.insert(m_population.end(), m_offspring.begin(), m_offspring.end());
    RollingSolution<Command>::DominanceSort(m_population);
    RollingSolution<Command>::CalculateCrowdedness(m_population);
    // choose solutions to be added to the next generation
    int rank_need_resort = m_population[m_population_size - 1].rank;
    if (m_population.size() > m_population_size && m_population[m_population_size].rank == rank_need_resort) // if next element is still of this rank, it means this rank can not be contained fully in current population, it needs selecting
    {
        // resort solutions of current rank
        Population::iterator bg = std::find_if(m_population.begin(), m_population.end(), [rank_need_resort](const Solution<Command> &s) { return rank_need_resort == s.rank; });
        Population::iterator ed = std::find_if(m_population.rbegin(), m_population.rend(), [rank_need_resort](const Solution<Command> &s) { return rank_need_resort == s.rank; }).base();
        std::sort(bg, ed, [](const Solution<Command> &l, const Solution<Command> &r) { return l.crowdedness > r.crowdedness; });
    }
    m_population.resize(m_population_size);
    return;
}

void RollingEA::InitFromObservation()
{
    m_enemy_team = m_observation->GetUnits(Unit::Alliance::Enemy);
    m_my_team = m_observation->GetUnits(Unit::Alliance::Self);
    m_game_info = m_observation->GetGameInfo();
    m_playable_dis = Vector2D(m_game_info.playable_max.x - m_game_info.playable_min.x, m_game_info.playable_max.y - m_game_info.playable_min.y);
    m_unit_types = m_observation->GetUnitTypeData();
}
#ifdef USE_GRAPHICS
void RollingEA::ShowOverallStatusGraphEachGeneration()
{
    EA::ShowOverallStatusGraphEachGeneration();
    std::cout << "diff of aves: " << m_history_objs_ave[0].back() - m_history_objs_ave[1].back() << std::endl;
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
#endif //USE_GRAPHICS

void RollingEA::GenerateOne(Solution<Command> &sol)
{
    size_t team_size = m_my_team.size();
    for (size_t i = 0; i < team_size; i++)
    {
        sol.variable[i].unit_tag = m_my_team[i]->tag;
        float move_dis_per_command = m_playable_dis.y / 3;
        float longest_map_bound = std::max(m_playable_dis.x, m_playable_dis.y);
        float moveable_radius = std::min(longest_map_bound, move_dis_per_command); //todo Think about the boundaries of the map!
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
            }
            action_raw.target_point = FixOutsidePointIntoRectangle(action_raw.target_point, m_game_info.playable_min, m_game_info.playable_max); // fix the target point outside the
            if (m_use_fix)
            {
                if (GetRandomFraction() < 0.7)
                {
                    action_raw.target_point = FixActionPosIntoEffectiveRangeToNearestEnemy(action_raw.target_point, m_unit_types[m_my_team[i]->unit_type].weapons.front().range * m_log_dis(m_random_engine), m_enemy_team);
                }
            }
        }
    }
}
void RollingEA::RecordObjectives()
{
    // all the objs
    int pop_sz = m_population.size();
    m_history_objs.emplace_back(std::vector<std::vector<float>>(pop_sz, std::vector<float>(m_objective_size)));
    std::vector<std::vector<float>> &current_generation_objs = m_history_objs.back();
    for (size_t i = 0; i < pop_sz; ++i)
    {
        current_generation_objs[i] = m_population[i].objectives;
    }
    for (size_t i = 0; i < m_objective_size; ++i) // the ith objective
    {
        // ave
        EA::m_history_objs_ave[i].push_back(std::abs(std::accumulate(current_generation_objs.begin(), current_generation_objs.end(), 0.f, [i](float initial_value, const std::vector<float> &so) -> float {
            return initial_value + so[i];
        })));
        EA::m_history_objs_ave[i].back() /= pop_sz;
        // best
        auto best_iter_i = std::max_element(current_generation_objs.begin(), current_generation_objs.end(), [i](const std::vector<float> &so1, const std::vector<float> so2) -> bool {
            return so1[i] < so2[i];
        });
        EA::m_history_objs_best[i].push_back(std::abs((*best_iter_i)[i]));
        // worst
        auto worst_iter_i = std::min_element(current_generation_objs.begin(), current_generation_objs.end(), [i](const std::vector<float> &so1, const std::vector<float> &so2) -> bool {
            return so1[i] < so2[i];
        });
        EA::m_history_objs_worst[i].push_back(std::abs((*worst_iter_i)[i]));
    }
    return;
}

void RollingEA::ActionAfterRun()
{
    Evaluate(m_population);
}

RollingSolution<Command> RollingEA::AssembleASolutionFromGoodUnits(const Population &evaluated_pop)
{
    RollingSolution<Command> assembled_solution(m_my_team.size(), m_objective_size);
    AssembleASolutionFromGoodUnits(assembled_solution, evaluated_pop);
    return assembled_solution;
}

void RollingEA::AssembleASolutionFromGoodUnits(RollingSolution<Command> &modified_solution, const Population &evaluated_pop)
{
    size_t pop_sz = evaluated_pop.size();
    for (size_t i = 0; i < m_my_team.size(); i++)
    {
        // todo search the pop to find the unit with greatest performence
        Population::const_iterator it_s = std::max_element(evaluated_pop.begin(), evaluated_pop.end(), [u_tag = m_my_team[i]->tag](const RollingSolution<Command> &first, const RollingSolution<Command> &second) -> bool {
            return first.aver_result.units_statistics.at(u_tag).attack_number < second.aver_result.units_statistics.at(u_tag).attack_number;
        });
        // use its command as a part of this solution
        //! in generating function, the order of variables in solution is set according to m_my_team's order
        std::vector<Command>::const_iterator it_c = std::find_if(it_s->variable.begin(), it_s->variable.end(), [u_tag = m_my_team[i]->tag](const Command &cmd) -> bool { return u_tag == cmd.unit_tag; }); // find the command of this unit in this solution
        if (it_c != it_s->variable.end())
        {
            modified_solution.variable[i] = *it_c; // the order is equal to my_team's order
        }
        else
        {
            throw(std::string("somethind wrong@") + __FUNCTION__);
        }
    }
}

Point2D RollingEA::FixActionPosIntoEffectiveRangeToNearestEnemy(const Point2D &action_target_pos, float effective_range, const Units &enemy_team)
{
    Point2D nearest_enemy_pos_to_target_point = SelectNearestUnitFromPoint(action_target_pos, enemy_team)->pos;
    if (Distance2D(action_target_pos, nearest_enemy_pos_to_target_point) < 2 * effective_range)
    {
        return action_target_pos;
    }
    return FixOutsidePointIntoCircle(action_target_pos, nearest_enemy_pos_to_target_point, effective_range);
}

} // namespace sc2