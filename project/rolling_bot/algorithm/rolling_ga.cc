#include"rolling_ga.h"
#include<thread>
#include<numeric>
#include<algorithm>
#include<iostream>

using namespace sc2;

void sc2::RollingGA::SetSimulators(const std::string& net_address, int port_start, const std::string& process_path, const std::string& map_path)
{
	assert(!m_simulators.empty());
	// Set all simulator's properties
	for (Simulator& sim:m_simulators)
	{
		// sim.SetNetAddress(net_address);
		sim.SetPortStart(port_start);
		sim.SetProcessPath(process_path);
		sim.SetMapPath(map_path);
		sim.SetStepSize(m_sims_step_size);
		// since there are up to two agent players, for simplicity I'd better make the port +2 at each time
		port_start += 2;
        // //! here for test
        // sim.LaunchStarcraft();
        // sim.StartGame();
    }
    std::vector<std::thread> start_game_threads(m_simulators.size());
	for (size_t i = 0; i < start_game_threads.size(); i++)
	{
		start_game_threads[i] = std::thread([&, i]()->void{
			m_simulators[i].LaunchStarcraft();
			// m_simulators[i].Connect(m_simulators[i].GetPortStart());
			m_simulators[i].StartGame();
		});
	}
	for (auto& t : start_game_threads) {
		t.join();
	}
}

void sc2::RollingGA::SetSimulatorsStart(const ObservationInterface* ob)
{
	SetObservation(ob);
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
		threads[i] = std::thread{ [&,i]() ->void {
			m_simulators[i].Run(m_run_length);
		} };
	}
	for (auto& t:threads)
	{
		t.join();
	}
}

void sc2::RollingGA::SetObservation(const ObservationInterface* observation)
{
	m_observation = observation;
	m_game_info = m_observation->GetGameInfo();
	m_unit_type = m_observation->GetUnitTypeData(); //? why isn't here anything wrong?
	m_my_team = m_observation->GetUnits(Unit::Alliance::Self);
	m_enemy_team = m_observation->GetUnits(Unit::Alliance::Enemy);
	m_playable_dis = Point2D(m_game_info.playable_max.x - m_game_info.playable_min.x, m_game_info.playable_max.y - m_game_info.playable_min.y);
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

void sc2::RollingGA::SetSimlatorsOpponent(const PlayerSetup& opponent)
{
	if (opponent.agent == nullptr) {
		for (auto& sim : m_simulators)
		{
			sim.SetOpponent(opponent.difficulty);
		}
	}
	else {
		for (auto& sim : m_simulators)
		{
			sim.SetOpponent(opponent.agent);
		}
	}
}

void RollingGA::SetSimulatorsMultithreaded(bool multithreaded)
{
	for (auto& sim:m_simulators)
	{
		sim.SetMultithreaded(multithreaded);
	}
}

void RollingGA::SetDebugMode(bool is_debug) {
    if (is_debug) {
        // m_debug_renderer.SetIsDisplay(true);
    } else {
        // m_debug_renderer.SetIsDisplay(false);
    }
    m_is_debug = is_debug;
}

Solution<Command> RollingGA::GenerateSolution() {
	//? These is a way to get avaliable abilities in s2client-api, but it is time consuming
	//? But for now, I choose to limit the chosen of abilities in only move and attack
	Solution<Command> sol(m_my_team.size(), m_evaluators.size());
	RawActions raw_actions(m_command_length);
	for (size_t i = 0; i < m_my_team.size(); i++)
	{
		sol.variable[i].unit_tag = m_my_team[i]->tag;
		float moveable_radius = MoveDistance(m_my_team[i], m_run_length* m_command_length, m_unit_type);
		Point2D current_location = m_my_team[i]->pos;
		sol.variable[i].actions.resize(m_command_length);
		for (ActionRaw& action_raw : sol.variable[i].actions)
		{ 
			// randomly choose to move or attack
			if (GetRandomFraction() < m_attack_possibility) {
				// randomly choose a location to attack...
				action_raw.ability_id = ABILITY_ID::ATTACK;
				action_raw.target_type = ActionRaw::TargetType::TargetPosition; //? pay attention here is my test code which need to changes
				action_raw.target_point = m_my_team[i]->pos + Point2DP(GetRandomFraction() * moveable_radius, GetRandomFraction() * 2 * PI).toPoint2D();
			}
			else {
				action_raw.ability_id = ABILITY_ID::MOVE;
				action_raw.target_type = ActionRaw::TargetType::TargetPosition;
				// construct move action
				action_raw.target_point += m_my_team[i]->pos + Point2DP(GetRandomFraction() * moveable_radius, GetRandomFraction() * 2 * PI).toPoint2D();
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
	assert(p.size() <= m_simulators.size());
	// use different simulators to evaluate solutions respectively
	// for each sim, copy the state and deploy the commands!
	for (size_t i = 0; i < p.size(); i++)
	{
		m_simulators[i].CopyAndSetState(m_observation);
#ifdef _DEBUG
		////? outputs units info that can indicate whether the CopyAndSet function has worked
		//Units us = m_simulators[i].Observation()->GetUnits();
		//sc2utility::output_units_health_in_order(us);
		//std::cout << std::endl;
#endif // _DEBUG
		m_simulators[i].SetOrders(p[i].variable);
	}
	RunSimulatorsSynchronous();
	//? output the best one for each generation, or outputs the average objectives for each generation
	float self_loss = 0, self_team_loss_total = 0, self_team_loss_best = std::numeric_limits<float>::max();
	float enemy_loss = 0, enemy_team_loss_total = 0, enemy_team_loss_best = std::numeric_limits<float>::lowest();
	for (size_t i = 0; i < p.size(); i++)
	{
		self_loss = m_simulators[i].GetTeamHealthLoss(Unit::Alliance::Self);
		enemy_loss = m_simulators[i].GetTeamHealthLoss(Unit::Alliance::Enemy);
		self_team_loss_total += self_loss;
		enemy_team_loss_total += enemy_loss;
		if (self_team_loss_best > self_loss) {
			self_team_loss_best = self_loss;
		}
		if (enemy_team_loss_best < enemy_loss) {
			enemy_team_loss_best = enemy_loss;
		}

		p[i].objectives[0] = m_simulators[i].GetTeamHealthLoss(Unit::Alliance::Enemy)/* - m_simulators[i].GetTeamHealthLoss(Unit::Alliance::Self)*/;
	}
	std::cout << "ally_team_loss_avg:\t" << self_team_loss_total / p.size() <<"\t"
		<< "ally_team_loss_best:\t" << self_team_loss_best << "\t"
		<< "enemy_team_loss_avg:\t" << enemy_team_loss_total / p.size() << "\t"
		<< "enemy_team_loss_best:\t" << enemy_team_loss_best << std::endl;
}

std::vector<const ObservationInterface*> RollingGA::GetAllSimsObservations() const {
    std::vector<const ObservationInterface*> observations(m_simulators.size());
    for (size_t i = 0; i < m_simulators.size(); i++) {
        observations[i] = m_simulators[i].Observation();
    }
    return observations;
}
