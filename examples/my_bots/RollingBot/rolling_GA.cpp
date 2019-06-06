#include "rolling_GA.h"
#include <thread>

using namespace sc2;

using Population = std::vector<Solution<Command>>;
using Evaluator = std::function<float(const std::vector<Command>&)>;
using Compare = std::function<bool(const Solution<Command>&, const Solution<Command>&)>;

//! According to known information generates solutions which is as valid as possiable 

//! before every time you run it, you should do it once



void sc2::RollingGA::SetSimulators(const std::string& net_address, int port_start, const std::string& process_path, const std::string& map_path, int step_size, const PlayerSetup& opponent, bool multithreaded)
{
	// Set all simulator's properties
	for (Simulator& sim:m_simulators)
	{
		sim.SetNetAddress(net_address);
		sim.SetPortStart(port_start);
		sim.SetProcessPath(process_path);
		if (opponent.agent == nullptr) {
			sim.SetOpponent(opponent.difficulty);
		}
		else {
			sim.SetOpponent(opponent.agent);
		}
		sim.SetMultithreaded(multithreaded);
		// run the game
		sim.StartGame();
		// since there are up to two agent players, for simplisity I'd better make the port +2 at each time
		port_start += 2;
	}
}

void sc2::RollingGA::SetSimulatorsStart(const ObservationInterface* ob)
{
	//todo try to implement them in multi threads
	for (Simulator& sim :m_simulators)
	{
		sim.CopyAndSetState(ob);
	}
}

void sc2::RollingGA::RunSimulatorsSynchronous()
{
	std::vector<std::thread> threads(m_simulators.size());
	for (size_t i = 0; i < threads.size(); i++)
	{
		threads[i] = std::thread{ [&]() ->void {
			m_simulators[i].Run(m_step_size);
		} };
	}
	for (auto& t:threads)
	{
		t.join();
	}
}

void sc2::RollingGA::SetGameInfo(const ObservationInterface* observation)
{
	m_observation = observation;
	m_game_info = m_observation->GetGameInfo();
	m_unit_type = m_observation->GetUnitTypeData(); //? why isn't here anything wrong?
	m_my_team = m_observation->GetUnits(Unit::Alliance::Self);
	m_enemy_team = m_observation->GetUnits(Unit::Alliance::Enemy);
	m_playable_dis = Point2D(m_game_info.playable_max.x - m_game_info.playable_min.x, m_game_info.playable_max.y - m_game_info.playable_min.y);
}

inline void sc2::RollingGA::SetPopulationSize(int population_size) {
	m_population_size = population_size;
	m_simulators.resize(m_population_size);
}

void sc2::RollingGA::SetSimulatorsStepSize(int step_size)
{
	for (Simulator& sim:m_simulators)
	{
		sim.SetStepSize.SetStepSize(step_size);
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
	//todo I need to construct a more robust simulator manager
	assert(p.size() < m_simulators.size());
	std::vector<std::thread> eva_threads(p.size());
	//todo use different simulators to evaluate solutions respectively
	RunSimulatorsSynchronous();
	for (size_t i = 0; i < p.size(); i++)
	{
		p[i].objectives[0] = m_simulators[i].GetTeamHealthLoss(Unit::Alliance::Enemy) - m_simulators[i].GetTeamHealthLoss(Unit::Alliance::Self);
	}
}
