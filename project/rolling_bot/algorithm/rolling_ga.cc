#include "rolling_ga.h"
#include <thread>
#include <future>
#include <chrono>
#include <numeric>
#include <algorithm>
#include <iostream>
#include "gnuplot-iostream.h"

#define MULTI_THREADED

using namespace sc2;

using Population = std::vector<Solution<Command>>;
using Evaluator = std::function<float(const std::vector<Command> &)>;
using Compare = std::function<bool(const Solution<Command> &, const Solution<Command> &)>;

void sc2::RollingGA::SetInfoFromObservation(const ObservationInterface *observation)
{
	m_observation = observation;
	m_game_info = m_observation->GetGameInfo();
	m_unit_type = m_observation->GetUnitTypeData(); //? why isn't here anything wrong?
	m_my_team = m_observation->GetUnits(Unit::Alliance::Self);
	m_enemy_team = m_observation->GetUnits(Unit::Alliance::Enemy);
	m_playable_dis = Vector2D(m_game_info.playable_max.x - m_game_info.playable_min.x, m_game_info.playable_max.y - m_game_info.playable_min.y);
}

void sc2::RollingGA::SetRunLength(int length)
{
	m_run_length = length;
}

void sc2::RollingGA::SetCommandLength(int command_length)
{
	m_command_length = command_length;
}

void sc2::RollingGA::SetAttackPossibility(float attack_possibility)
{
	m_attack_possibility = attack_possibility;
}

void RollingGA::SetDebugMode(bool is_debug)
{
	m_is_debug = is_debug;
}

void RollingGA::InitBeforeRun()
{
	// call init() of the father class
	GA::InitBeforeRun();
	SetInfoFromObservation(m_observation);
	m_self_team_loss_ave.clear();
	m_self_team_loss_best.clear();
	m_enemy_team_loss_ave.clear();
	m_enemy_team_loss_best.clear();
	// m_gp << "set title 'Algorithm Status'" << std::endl;
	m_line_chart_renderer.SetTitle("Algorithm Status");
	m_line_chart_renderer.SetXRange(0.f, m_max_generation);
	// m_gp << "set xrange [0:" << m_max_generation << "]" << std::endl;
}

Solution<Command> RollingGA::GenerateSolution()
{
	//? These is a way to get avaliable abilities in s2client-api, but it is time-consuming
	//? But for now, I choose to limit the chosen of abilities in only move and attack
	size_t team_size = m_my_team.size();
	Solution<Command> sol(team_size, m_objective_size);
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
	return sol;
}

void sc2::RollingGA::Mutate(Solution<Command> &s)
{
	// the user must ensure that the actions is not empty
	ActionRaw &action = GetRandomEntry(GetRandomEntry(s.variable).actions);
	if (action.ability_id == ABILITY_ID::ATTACK)
	{
		// todo if the action is attack, move the target point to the mass center of the enemies / one weakest enemy / nearest enemy / one random unit
		// action.target_point += SelectNearestUnitFromPoint(action.target_point, m_observation->GetUnits(Unit::Alliance::Enemy));
		Vector2D from_p_to_e = GetRandomEntry(m_enemy_team)->pos - action.target_point;
		action.target_point += from_p_to_e * GetRandomFraction();
	}
	else
	{
		// todo if the action is move
		action.target_point += Point2D(GetRandomInteger(-1, 1) * m_playable_dis.x, GetRandomInteger(-1, 1) * m_playable_dis.y) / m_mutate_step;
	}
	action.target_point = FixOutsidePointIntoMap(action.target_point, m_game_info.playable_min, m_game_info.playable_max);
}
Population sc2::RollingGA::CrossOver(const Solution<Command> &a, const Solution<Command> &b)
{
	// random select one unit
	int unit_index = GetRandomInteger(0, a.variable.size() - 1);
	// exchange the subarray at the same pos in two units' commands
	Population offspring({a, b});
	size_t order_size = a.variable[unit_index].actions.size();
	size_t start = sc2::GetRandomInteger(0, order_size - 1);
	size_t end = sc2::GetRandomInteger(0, order_size - 1);
	if (start > end)
	{
		std::swap(start, end);
	}
	for (size_t i = start; i < end; i++)
	{
		ActionRaw &action1 = offspring[0].variable[unit_index].actions[i];
		ActionRaw &action2 = offspring[0].variable[unit_index].actions[i];
		std::swap(action1, action2);
		//todo randomly use the quantile points of two actions to set the target posisiton
		if (action1.ability_id == action2.ability_id && action1.target_type == ActionRaw::TargetType::TargetPosition && action2.target_type == ActionRaw::TargetType::TargetPosition)
		{
			if (GetRandomFraction() < 0.5)
			{
				Point2D pos1 = action1.target_point;
				Point2D pos2 = action2.target_point;
				action1.target_point = 0.75f * pos1 + 0.25f * pos2;
				action2.target_point = 0.25f * pos1 + 0.75f * pos2;
			}
		}
	}
	return offspring;
}

void sc2::RollingGA::Evaluate(Population &pop)
{

#ifdef MULTI_THREADED
	m_simulation_pool.CopyStateAndSendOrdersAsync(m_observation, pop);
	if (m_is_debug)
	{
		m_simulation_pool.RunSimsAsync(m_run_length, m_debug_renderers);
	}
	else
	{
		m_simulation_pool.RunSimsAsync(m_run_length);
	}
#else
	//todo one by one?
#endif

	// output the best one and the average objectives for each generation for each generation
	float self_loss = 0, self_team_loss_total = 0, self_team_loss_best = std::numeric_limits<float>::max();
	float enemy_loss = 0, enemy_team_loss_total = 0, enemy_team_loss_best = std::numeric_limits<float>::lowest();
	size_t sz = pop.size();
	for (size_t i = 0; i < sz; i++)
	{
		self_loss = m_simulation_pool.GetTeamHealthLoss(i, Unit::Alliance::Self);
		enemy_loss = m_simulation_pool.GetTeamHealthLoss(i, Unit::Alliance::Enemy);

		// set the 2 objectives
		pop[i].objectives[1] = -self_loss;
		pop[i].objectives[0] = enemy_loss;

		// calculate the average and the best
		self_team_loss_total += self_loss;
		enemy_team_loss_total += enemy_loss;
		if (self_team_loss_best > self_loss)
		{
			self_team_loss_best = self_loss;
		}
		if (enemy_team_loss_best < enemy_loss)
		{
			enemy_team_loss_best = enemy_loss;
		}
	}
	// output the the results
	std::cout << "ally_team_loss_avg:\t" << self_team_loss_total / pop.size() << "\t"
			  << "ally_team_loss_best:\t" << self_team_loss_best << "\t"
			  << "enemy_team_loss_avg:\t" << enemy_team_loss_total / pop.size() << "\t"
			  << "enemy_team_loss_best:\t" << enemy_team_loss_best << std::endl;
	// store all the data
	m_self_team_loss_ave.push_back(self_team_loss_total / pop.size());
	m_self_team_loss_best.push_back(self_team_loss_best);
	m_enemy_team_loss_ave.push_back(enemy_team_loss_total / pop.size());
	m_enemy_team_loss_best.push_back(enemy_team_loss_best);
}

void RollingGA::ShowGraphEachGeneration()
{
	//todo I can use another thread to do this display
	// set the index here;
	std::vector<float> indices(m_current_generation + 1);
	std::iota(indices.begin(), indices.end(), 0);
	// set all the lines to be showed
	// m_gp << "set style func linespoints" << std::endl;
	// m_gp << "plot" << m_gp.file1d(std::make_tuple(indices, m_self_team_loss_ave)) << "with lines title 'self lost ave',"
	// 	 << m_gp.file1d(std::make_tuple(indices, m_self_team_loss_best)) << "with lines title 'self lost best',"
	// 	 << m_gp.file1d(std::make_tuple(indices, m_enemy_team_loss_ave)) << "with lines title 'enemy lost ave',"
	// 	 << m_gp.file1d(std::make_tuple(indices, m_enemy_team_loss_best)) << std::string("with lines title 'enemy lost best',")
	// 	 << std::endl;
	// show the graph of algorithm status
	std::vector<std::vector<float>> data{m_self_team_loss_ave, m_self_team_loss_best, m_enemy_team_loss_ave, m_enemy_team_loss_best};
	std::vector<std::string> names{"self lost ave", "self lost best", "enemy lost ave", "enemy lost best"};
	m_line_chart_renderer.Show(data, indices, names);
	// show the graph of the objectives of all solutions
	if (!m_population.empty())
	{
		size_t obj_sz = m_population.front().objectives.size();
		std::vector<std::vector<float>> objs(m_population_size, m_population.front().objectives);
		for (size_t i = 1; i < m_population_size; i++)
		{
			objs[i] = m_population.at(i).objectives;
		}
		// m_gp_mo << "set xrange [-300:0]\nset yrange [0:300]\n";
		// todo set the colors of the first three solution set
		// m_gp_mo << "set xlabel 'damage to enemy'" << std::endl
		// 		<< " set ylabel 'damage to me'" << std::endl;
		// m_gp_mo << "plot" << m_gp_mo.file1d(objs)
		// 		<< "with points lc rgb 'red' pt 4 title 'current individuals'," << m_gp_mo.file1d(m_last_solution_dis) << "with points lc rgb 'blue' pt 1 title 'individuals of last generation'" << std::endl;
		std::list<std::vector<std::vector<float>>> data{m_last_solution_dis, objs};
		std::vector<std::string> names{"individuals of last generation", "current individuals"};
		m_objectives_distribution_graph.Show(data, names);
		m_last_solution_dis = objs;
	}
}