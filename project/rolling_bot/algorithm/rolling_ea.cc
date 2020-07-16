#include "global_defines.h"

#include <fstream>

#include "rolling_ea.h"
#include <sc2lib/sc2_utils.h>
#include "../../debug_use.h"

namespace sc2
{

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
    {                          // doesn't call the base class's Init function
        InitFromObservation(); // set the m_my_team and some other things
        for (int i = 0; i < m_populations.size(); ++i)
        {
            int variable_size = ((i == 0) ? m_my_team.size() : m_enemy_team.size());
            for (RollingSolution<Command> &sol : m_populations[i])
            {
                sol.variable.resize(variable_size);
                sol.objectives.resize(m_objective_size);
            }
        }
    }

    void RollingEA::Generate()
    {
        for (size_t i = 0; i < m_populations.size(); ++i)
        {
            for (size_t j = 0; j < m_population_size; ++j)
            {
                GenerateOne(m_populations[i][j]);
            }
        }

        if (m_use_priori) // generate some solutions with priori knowledge
        {
            int enemy_sz = std::min(m_enemy_team.size(), (size_t)m_population_size / 5);
            for (size_t i = 0; i < enemy_sz; ++i)
            {
                RollingSolution<Command> &random_sol = m_populations[0][i];
                Point2D target_position = m_enemy_team[i]->pos;
                int unit_sz = random_sol.variable.size();
                for (size_t j = 0; j < unit_sz; ++j)
                {
                    const Unit *unit = m_observation->GetUnit(random_sol.variable[j].unit_tag); //todo m_enemy_team needed, I should try my best not to use m_observation during algorithm run
                    RawActions &actions = random_sol.variable[j].actions;
                    int action_sz = actions.size();
                    for (size_t k = 0; k < action_sz; k++)
                    {
                        Vector2D u_to_e = target_position - unit->pos;
                        actions[k].target_point = unit->pos + u_to_e * ((u_to_e.modulus() - m_unit_types[unit->unit_type].weapons.front().range) / u_to_e.modulus());
                    }
                }
                random_sol.is_priori = true;
            }
        }
    }

    void RollingEA::Evaluate()
    {
        if (m_populations.size() == 1)
        {
            if (m_current_generation == 0)
            {
                Evaluate(m_populations[0]);
            }
            else
            {
                Evaluate(m_offsprings[0]);
            }
        }
        else
        {
            if (m_current_generation == 0)
            {
                Evaluate(m_populations[0], m_populations[1], m_sub_pop_size);
            }
            else
            {
                Evaluate(m_offsprings[0], m_offsprings[1], m_sub_pop_size);
            }
        }
    }

    void RollingEA::Evaluate(Population &pop)
    {
        if (m_objective_size == 3)
        {
            // get initial total health of both team from observation
            float total_health_me = GetTotalHealth(m_observation->GetUnits(Unit::Alliance::Self));
            float total_health_enemy = GetTotalHealth(m_observation->GetUnits(Unit::Alliance::Enemy));
            size_t pop_sz = pop.size();
            for (auto &item : pop)
            {
                item.ClearSimData();
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
                    {                                                                                                      // record game results
                        const std::map<Tag, const Unit *> &units_correspondence = m_simulation_pool[i].GetRelativeUnits(); // Units final state in simulation
#ifdef DEBUG
                        if (i + 1 < pop_sz && m_simulation_pool[i].GetRelativeUnits().size() != m_simulation_pool[i + 1].GetRelativeUnits().size())
                        {
                            std::cout << "some mistake in return units map" << std::endl;
                        }
#endif // DEBUG
                        for (const auto &unit : units_correspondence)
                        {
                            // pop[i].results[j].units[unit.first];s
                            pop[i].results[j].units[unit.first].final_state = *(unit.second);
                            pop[i].results[j].units[unit.first].statistics = m_simulation_pool[i].GetUnitStatistics(unit.first);
                        }
                        pop[i].results[j].game.end_loop = m_simulation_pool[i].GetEndLoop();
                        pop[i].results[j].game.result = m_simulation_pool[i].CheckGameResult();
                        pop[i].CalculateAver(); // based on the recorded statistics, calculate the average results
                    }
                    { // set the objectives
                        float enemy_loss = m_simulation_pool.GetTeamHealthLoss(i, Unit::Alliance::Enemy);
                        float my_loss = m_simulation_pool.GetTeamHealthLoss(i, Unit::Alliance::Self);
                        pop[i].objectives[0] += m_simulation_pool.GetTeamHealthLoss(i, Unit::Alliance::Enemy);
                        pop[i].objectives[1] += -m_simulation_pool.GetTeamHealthLoss(i, Unit::Alliance::Self);
                        pop[i].aver_result.total_health_loss_enemy += enemy_loss;
                        pop[i].aver_result.total_health_loss_mine += my_loss;
                        if (pop[i].results[j].game.result != GameResult::Win) // maximization
                        {
                            pop[i].objectives[2] -= m_sim_length;
                            pop[i].aver_result.win_loop += m_sim_length;
                            // std::cout << pop[i].aver_result.win_loop << '\t' << m_sim_length << std::endl;
                        }
                        else
                        {
                            pop[i].objectives[2] -= pop[i].results[j].game.end_loop;
                            pop[i].aver_result.win_loop += pop[i].results[j].game.end_loop;
                        }
                    }
                }
            }
            // write the real obj values to those solutions
            for (size_t i = 0; i < pop_sz; i++)
            {
                pop[i].objectives[0] /= m_evaluation_time_multiplier;
                pop[i].objectives[1] /= m_evaluation_time_multiplier; // transform it to maximum optimization
                pop[i].objectives[2] /= m_evaluation_time_multiplier;
                pop[i].aver_result.total_health_loss_enemy /= m_evaluation_time_multiplier;
                pop[i].aver_result.total_health_loss_mine /= m_evaluation_time_multiplier;
                pop[i].aver_result.win_loop /= m_evaluation_time_multiplier;
            }
        }
        else if (m_objective_size == 2)
        {
            // get initial total health of both team from observation
            float total_health_me = GetTotalHealth(m_observation->GetUnits(Unit::Alliance::Self));
            float total_health_enemy = GetTotalHealth(m_observation->GetUnits(Unit::Alliance::Enemy));
            size_t pop_sz = pop.size();
            for (auto &item : pop)
            {
                item.ClearSimData();
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
                    try
                    {
                        /* code */
                        m_simulation_pool.RunSimsAsync(m_sim_length);
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << e.what() << '\n'; // there has been a timeout handler in RunSimsAsync()
                    }
                }
                size_t pop_sz = pop.size();
                for (size_t i = 0; i < pop_sz; i++)
                {
                    {                                                                                                      // record game results
                        const std::map<Tag, const Unit *> &units_correspondence = m_simulation_pool[i].GetRelativeUnits(); // Units final state in simulation
#ifdef DEBUG
                        if (i + 1 < pop_sz && m_simulation_pool[i].GetRelativeUnits().size() != m_simulation_pool[i + 1].GetRelativeUnits().size())
                        {
                            std::cout << "some mistake in return units map" << std::endl;
                        }
#endif // DEBUG
                        for (const auto &unit : units_correspondence)
                        {
                            pop[i].results[j].units[unit.first];
                            pop[i].results[j].units[unit.first].final_state = *(unit.second);
                            pop[i].results[j].units[unit.first].statistics = m_simulation_pool[i].GetUnitStatistics(unit.first);
                        }
                        pop[i].results[j].game.end_loop = m_simulation_pool[i].GetEndLoop();
                        pop[i].results[j].game.result = m_simulation_pool[i].CheckGameResult();
                        pop[i].CalculateAver(); // based on the recorded statistics, calculate the average results
                    }
                    { // set the objectives
                        float enemy_loss = m_simulation_pool.GetTeamHealthLoss(i, Unit::Alliance::Enemy);
                        float my_loss = m_simulation_pool.GetTeamHealthLoss(i, Unit::Alliance::Self);
                        pop[i].objectives[0] += enemy_loss / total_health_enemy - my_loss / total_health_me;
                        pop[i].aver_result.total_health_loss_enemy += enemy_loss;
                        pop[i].aver_result.total_health_loss_mine += my_loss;
                        // pop[i].objectives[1] += -m_simulation_pool.GetTeamHealthLoss(i, Unit::Alliance::Self);
                        if (pop[i].results[j].game.result != GameResult::Win) // maximization
                        {
                            pop[i].objectives[1] -= m_sim_length;
                            pop[i].aver_result.win_loop += m_sim_length;
                        }
                        else
                        {
                            pop[i].objectives[1] -= pop[i].results[j].game.end_loop;
                            pop[i].aver_result.win_loop += pop[i].results[j].game.end_loop;
                        }
                    }
                }
            }
            // write the real obj values to those solutions
            for (size_t i = 0; i < pop_sz; i++)
            {
                pop[i].objectives[0] /= m_evaluation_time_multiplier;
                // pop[i].objectives[1] /= m_evaluation_time_multiplier; // transform it to maximum optimization
                pop[i].objectives[1] /= m_evaluation_time_multiplier;
                pop[i].aver_result.total_health_loss_enemy /= m_evaluation_time_multiplier;
                pop[i].aver_result.total_health_loss_mine /= m_evaluation_time_multiplier;
                pop[i].aver_result.win_loop /= m_evaluation_time_multiplier;
            }
        }
        return;
    }

    void RollingEA::Evaluate(Population &my_pop, Population enemy_pop, int sub_pop_size)
    {
    }

    void RollingEA::Select()
    {
        for (int i = 0; i < m_populations.size(); ++i)
        {
            m_populations[i].insert(m_populations[i].end(), m_offsprings[i].begin(), m_offsprings[i].end());
            RollingSolution<Command>::DominanceSort<RollingSolution>(m_populations[i], RollingSolution<Command>::RollingLess);
            // choose solutions to be added to the next generation
            int rank_need_resort = m_populations[i][m_population_size - 1].rank;
            if (m_populations[i].size() > m_population_size && m_populations[i][m_population_size].rank == rank_need_resort) // if next element is still of this rank, it means this rank can not be contained fully in current population, it needs selecting
            {
                // resort solutions of current rank
                Population::iterator bg = std::find_if(m_populations[i].begin(), m_populations[i].end(), [rank_need_resort](const RollingSolution<Command> &s) { return rank_need_resort == s.rank; });
                Population::iterator ed = std::find_if(m_populations[i].rbegin(), m_populations[i].rend(), [rank_need_resort](const RollingSolution<Command> &s) { return rank_need_resort == s.rank; }).base();
                RollingSolution<Command>::CalculateCrowdedness(m_populations[i], std::distance(m_populations[i].begin(), bg), std::distance(m_populations[i].begin(), ed));
                std::stable_sort(bg, ed, [](const RollingSolution<Command> &l, const RollingSolution<Command> &r) { return l.crowdedness > r.crowdedness; });
            }
            m_populations[i].resize(EA::m_population_size);
        }
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

    void RollingEA::GenerateOne(RollingSolution<Command> &sol, int pop_index)
    { //todo need to be modified
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
        int pop_sz = m_populations[0].size();
        m_history_objs.emplace_back(std::vector<std::vector<float>>(pop_sz, std::vector<float>(m_objective_size)));
        std::vector<std::vector<float>> &current_generation_objs = m_history_objs.back();
        for (size_t i = 0; i < pop_sz; ++i)
        {
            current_generation_objs[i] = m_populations[0][i].objectives;
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
        if (m_is_output_file)
        {
            if (m_output_file_path.empty())
            {
                m_output_file_path = CurrentFolder() + "/obj_record.txt";
            }
            std::fstream fs = std::fstream(m_output_file_path, std::ios::app | std::ios::out);
            EA::OutputAllHistoryObjectives(fs);
            fs.close();
        }
    }

    RollingSolution<Command> RollingEA::AssembleASolutionFromGoodUnits(const Population &evaluated_pop)
    {
        RollingSolution<Command> assembled_solution(m_my_team.size(), m_objective_size);
        AssembleASolutionFromGoodUnits(assembled_solution, evaluated_pop);
        return assembled_solution;
    }

    void RollingEA::AssembleASolutionFromGoodUnits(RollingSolution<Command> &modified_solution, const Population &evaluated_pop)
    {
        if (m_my_team.size() == 1) // no use
        {
            return;
        }
        modified_solution.ClearSimData();
        modified_solution.variable.clear();
        modified_solution.variable.resize(m_my_team.size());
        modified_solution.objectives.clear();
        size_t pop_sz = evaluated_pop.size();
        for (size_t i = 0; i < m_my_team.size(); i++)
        {
            // todo search the pop to find the unit with greatest performence
            Population::const_iterator it_s = std::max_element(evaluated_pop.begin(), evaluated_pop.end(), [u_tag = m_my_team[i]->tag](const RollingSolution<Command> &first, const RollingSolution<Command> &second) -> bool {
                const UnitStatisticalData &first_data = first.aver_result.units_statistics.at(u_tag), second_data = second.aver_result.units_statistics.at(u_tag);
                if (first_data.attack_number < second_data.attack_number ||
                    (first_data.attack_number == second_data.attack_number && first_data.health_change < second_data.health_change))
                {
                    return true;
                }
                else
                {
                    return false;
                }
            });
            // use its command as a part of this solution
            //! in generating function, the order of variables in solution is set according to m_my_team's order
            std::vector<Command>::const_iterator it_c = std::find_if(it_s->variable.begin(), it_s->variable.end(), [u_tag = m_my_team[i]->tag](const Command &cmd) -> bool { return u_tag == cmd.unit_tag; }); // find the command of this unit in this solution
            if (it_c != it_s->variable.end())
            {
                modified_solution.variable[i] = *it_c; // the order is equal to my_team's order //? why it can not be copied?
            }
            else
            {
                throw(std::string("somethind wrong@") + __FUNCTION__);
            }
        }
        modified_solution.is_priori = true;
    }

    Point2D RollingEA::FixActionPosIntoEffectiveRangeToNearestEnemy(const Point2D &action_target_pos, float effective_range, const Units &enemy_team)
    {
        Point2D nearest_enemy_pos_to_target_point = FindNearestUnitFromPoint(action_target_pos, enemy_team)->pos;
        if (Distance2D(action_target_pos, nearest_enemy_pos_to_target_point) < 2 * effective_range)
        {
            return action_target_pos;
        }
        return FixOutsidePointIntoCircle(action_target_pos, nearest_enemy_pos_to_target_point, effective_range);
    }

    RollingSolution<Command> RollingEA::FixBasedOnSimulation(const RollingSolution<Command> &parent)
    {
        RollingSolution<Command> child = parent;
        // find the unattacked move
        if (child.results.size() == 1)
        {
            const auto &unit_stat = child.results.front().units;
            std::vector<Command> &vars = child.variable;
            for (Command &var : vars)
            {
                Tag tag = var.unit_tag;
                float attack_range = m_unit_types[m_observation->GetUnit(tag)->unit_type].weapons.front().range;
                const std::list<Events::Action> &actions = unit_stat.at(tag).statistics.events.actions;
                int move_count = 0;
                std::list<Events::Action>::const_iterator it2 = actions.begin();
                for (std::list<Events::Action>::const_iterator it = actions.begin(); it != actions.end(); ++it)
                {
                    it2 = it;
                    ++it2;
                    if (it->ability() == ABILITY_ID::MOVE)
                    {
                        //todo check if out of range
                        if (it2->ability() != ABILITY_ID::ATTACK_ATTACK)
                        {
                            //todo get the positions of enemies
                            std::vector<Point2D> possible_enemey_pos(m_enemy_team.size());
                            for (int i = 0; i < m_enemy_team.size(); ++i)
                            {
                                possible_enemey_pos[i] = child.GetUnitPossiablePosition(m_enemy_team[i]->tag, it->gameLoop());
                            }
                            //todo find the nearest one
                            Point2D target = FindNearestPointFromPoint(it->position(), possible_enemey_pos);
                            //todo check if the unit is out of range
                            if (Distance2D(target, it->position()) > attack_range * 1.5)
                            {
                                Point2D fix = FixOutsidePointIntoCircle(var.actions[move_count].target_point, target, attack_range * 1.5);
                                var.actions[move_count].target_point = fix;
                            }
                        }
                        ++move_count;
                    }
                }
            }
        }
        return child;
    }

    void RollingEA::OutputPopulationStat(std::ostream &os, int pop_index)
    {
        //todo aver and max
        float damage_aver = std::accumulate(m_populations[pop_index].begin(), m_populations[pop_index].end(), 0.f, [](float init, const RollingSolution<Command> &r) -> float {
                                return init + r.aver_result.total_health_loss_enemy;
                            }) /
                            m_populations[pop_index].size();
        auto damage_max_it = std::max_element(m_populations[pop_index].begin(), m_populations[pop_index].end(), [](const RollingSolution<Command> &l, const RollingSolution<Command> &r) -> bool {
            return l.aver_result.total_health_loss_enemy < r.aver_result.total_health_loss_enemy;
        });
        float hurt_aver = std::accumulate(m_populations[pop_index].begin(), m_populations[pop_index].end(), 0.f, [](float init, const RollingSolution<Command> r) -> float {
                              return init + r.aver_result.total_health_loss_mine;
                          }) /
                          m_populations[pop_index].size();
        auto hurt_min_it = std::max_element(m_populations[pop_index].begin(), m_populations[pop_index].end(), [](const RollingSolution<Command> &l, const RollingSolution<Command> &r) -> bool {
            return l.aver_result.total_health_loss_mine > r.aver_result.total_health_loss_mine;
        });
        uint32_t win_loop = std::accumulate(m_populations[pop_index].begin(), m_populations[pop_index].end(), 0.f, [](float init, const RollingSolution<Command> &r) -> float {
                                return init + r.aver_result.win_loop;
                            }) /
                            m_populations[pop_index].size();
        auto win_loop_min_it = std::max_element(m_populations[pop_index].begin(), m_populations[pop_index].end(), [](const RollingSolution<Command> &l, const RollingSolution<Command> &r) -> bool {
            return l.aver_result.win_loop > r.aver_result.win_loop;
        });
        os << damage_aver << '\t' << damage_max_it->aver_result.total_health_loss_enemy << '\t' << hurt_aver << '\t' << hurt_min_it->aver_result.total_health_loss_mine << '\t' << win_loop << '\t' << win_loop_min_it->aver_result.win_loop << std::endl;
    }

    void RollingEA::OutputPopulationSimResult(std::ostream &os, int pop_index) const
    {
        for (auto &&s : m_populations[0])
        {
            float total_health_enemy = GetTotalHealth(m_observation->GetUnits(Unit::Alliance::Enemy));
            float total_health_me = GetTotalHealth(m_observation->GetUnits(Unit::Alliance::Self));
            os << s.aver_result.total_health_loss_enemy << '\t' << s.aver_result.total_health_loss_mine << '\t' << s.aver_result.win_loop << '\t' << total_health_enemy << '\t' << total_health_me << '\t' << s.rank << std::endl;
        }
        os << std::endl;
    }

    std::string RollingEA::GetSettingString()
    {
        return std::to_string(m_objective_size) + '\t' + std::to_string(m_population_size) + '\t' + std::to_string(std::static_pointer_cast<MGT>(TerminationCondition(TERMINATION_CONDITION::MAX_GENERATION))->MaxGeneration()) + '\t' + std::to_string(m_use_priori) + '\t' + std::to_string(m_use_fix_by_data) + '\t' + std::to_string(m_use_assemble) + '\t' + std::to_string(m_sim_length);
    }

} // namespace sc2