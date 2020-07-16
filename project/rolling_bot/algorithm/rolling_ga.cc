#include "rolling_ga.h"
#include <thread>
#include <future>
#include <chrono>
#include <numeric>
#include <algorithm>
#include <iostream>
#include "../contrib/gnuplot-iostream.h"

#define MULTI_THREADED

using namespace sc2;

using Population = std::vector<RollingSolution<Command>>;

void RollingGA::InitBeforeRun()
{
	EA::InitBeforeRun();
	GA::InitOnlySelfMemeberBeforeRun();
	RollingEA::InitOnlySelfMembersBeforeRun();
	InitOnlySelfMemeberBeforeRun();
}

void RollingGA::InitOnlySelfMemeberBeforeRun()
{
	//nothing
}

void RollingGA::Mutate()
{
	int children_sz = m_offsprings[0].size();
	for (RollingSolution<Command> &sol : m_populations[0])
	{
		if (GetRandomFraction() < GA::m_mutation_rate)
		{
			Mutate_(sol);
		}
	}
}

void RollingGA::Crossover()
{
	if (m_offsprings.size() != m_populations.size())
	{
		m_offsprings.resize(m_populations.size());
	}
	for (size_t i = 0; i < m_offsprings.size(); i++)
	{
		m_offsprings[i].clear();
		m_offsprings[i].reserve(m_population_size * 2); // Errr, for easy use
	}

	for (size_t i = 0; i < m_populations.size(); i++)
	{
		for (size_t j = 0; j < m_population_size; j += 2)
		{
			Population temp_children = Crossover_(EA::m_populations[i][j], EA::m_populations[i][GetRandomInteger(0, m_population_size - 1)]);
			EA::m_offsprings[i].insert(m_offsprings[i].end(), temp_children.cbegin(), temp_children.cend());
		}
	}
}

void RollingGA::Mutate_(RollingSolution<Command> &s)
{
	// the user must ensure that the actions is not empty
	ActionRaw &action = GetRandomEntry(GetRandomEntry(s.variable).actions);
	if (action.ability_id == ABILITY_ID::ATTACK_ATTACK || action.ability_id == ABILITY_ID::ATTACK)
	{
		// todo if the action is attack, move the target point to the mass center of the enemies / one weakest enemy / nearest enemy / one random unit
		Vector2D from_p_to_e = GetRandomEntry(RollingEA::m_enemy_team)->pos - action.target_point;
		action.target_point += from_p_to_e * GetRandomFraction();
	}
	else
	{
		// todo if the action is move
		action.target_point += Point2D(GetRandomInteger(-1, 1) * m_playable_dis.x, GetRandomInteger(-1, 1) * m_playable_dis.y) / m_mutate_step;
	}
	action.target_point = FixOutsidePointIntoRectangle(action.target_point, m_game_info.playable_min, m_game_info.playable_max);
}

Population RollingGA::Crossover_(const RollingSolution<Command> &a, const RollingSolution<Command> &b)
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