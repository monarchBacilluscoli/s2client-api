#include "rolling_GA.h"

using namespace sc2;

using Population = std::vector<Solution<Command>>;
using Evaluator = std::function<float(const std::vector<Command>&)>;
using Compare = std::function<bool(const Solution<Command>&, const Solution<Command>&)>;

//! According to known information generates solutions which is as valid as possiable 

//! before every time you run it, you should do it once

void sc2::RollingGA::SetSimulators(std::string net_address, int port_start, std::string map_path, int step_size, const PlayerSetup& opponent, bool Multithreaded)
{
	//todo Set all simulator's properties
	for (size_t i = 0; i < m_population_size; i++)
	{
		
	}
}

Solution<Command> RollingGA::GenerateSolution() {
	//? no, there is no way for me to get the abilities avaliable
	//? limit the chosen of abilities in only move and attack
	Solution<Command> sol(m_my_team.size(), m_evaluators.size());
	//todo for each unit
	RawActions raw_actions(m_command_size);
	for (size_t i = 0; i < m_my_team.size(); i++)
	{
		sol.variable[i].unit_tag = m_my_team[i]->tag;
		float moveable_radius = sc2utility::move_distance(m_my_team[i], m_step_size* m_command_size, m_unit_type);
		Point2D current_location = m_my_team[i]->pos;
		sol.variable[i].actions.resize(m_command_size);
		for (ActionRaw& action_raw : sol.variable[i].actions)
		{ 
			//todo randomly choose to move or attack
			if (GetRandomFraction() < m_attack_possibility) {
				//todo randomly choose a location to attack...
				action_raw.ability_id = ABILITY_ID::ATTACK;
				action_raw.target_type = ActionRaw::TargetType::TargetPosition; //? pay attention here is my test code which need to changes
				action_raw.target_point = m_my_team[i]->pos + Point2DInPolar(GetRandomFraction() * moveable_radius, GetRandomFraction() * 2 * PI).toPoint2D();
			}
			else {
				action_raw.ability_id = ABILITY_ID::MOVE;
				action_raw.target_type = ActionRaw::TargetType::TargetPosition;
				//todo construct move action
				//todo choose a random point in it... add it to last time's location
				action_raw.target_point += m_my_team[i]->pos + Point2DInPolar(GetRandomFraction() * moveable_radius, GetRandomFraction() * 2 * PI).toPoint2D();
			}
		}
	}
	return sol;
}

void sc2::RollingGA::Mutate(Solution<Command>& s)
{
	// the user must ensure that the actions is not empty
	ActionRaw& action = GetRandomEntry(GetRandomEntry(s.variable).actions);
	action.target_point += Point2D(GetRandomInteger(-1, 1) * m_playable_dis.x, GetRandomInteger(-1, 1) * m_playable_dis.y) / m_mutate_step;
}

void sc2::RollingGA::Evaluate(Population& p)
{
	//todo use multiple simulator to evaluate those solutions in p
	//? But I should rewrite the initialization and constructor first
	//? And of course, before that, I need to rewrite the constructor of simulator, I need it can automatically launch SC2 instance remotely

	//todo use different simulators to evaluate solutions respectively

}
