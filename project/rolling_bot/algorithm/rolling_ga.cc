#include"rolling_ga.h"
#include<thread>
#include<numeric>
#include<algorithm>
#include<iostream>
#include"gnuplot-iostream.h"

#define MULTI_THREADED

using namespace sc2;

using Population = std::vector<Solution<Command>>;
using Evaluator = std::function<float(const std::vector<Command> &)>;
using Compare = std::function<bool(const Solution<Command> &, const Solution<Command> &)>;

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
    std::cout << m_simulators.size() << std::endl;
    for (size_t i = 0; i < m_simulators.size(); i++) {
        m_simulators[i].CopyAndSetState(ob);
    }
}

void sc2::RollingGA::RunSimulatorsSynchronous() 
{
    std::vector<std::thread> threads(m_simulators.size());
    if (m_is_debug) {
        for (size_t i = 0; i < threads.size(); i++) {
            threads[i] = std::thread{[&, i]() -> void {
                m_simulators[i].Run(m_run_length, &m_debug_renderers[i]);
            }};
            // if debug flag is on, the debug renderes are involved
        }
    } else {
        std::vector<std::thread> threads(m_simulators.size());
        for (size_t i = 0; i < threads.size(); i++) {
            threads[i] = std::thread{[&, i]() -> void {
                m_simulators[i].Run(m_run_length);
            }};
        }
    }
    for (auto& t : threads) {
        t.join();
    }
}

void sc2::RollingGA::RunSimulatorsOneByOne() {
    int count = m_simulators.size();
    if (m_is_debug) {
        for (size_t i = 0; i < count; i++) {
            m_simulators[i].Run(m_run_length, &m_debug_renderers[i]);
        }
    } else {
        for (size_t i = 0; i < count; i++) {
            m_simulators[i].Run(m_run_length);
        }
    }
}

void sc2::RollingGA::SetObservation(const ObservationInterface* observation)
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

void RollingGA::InitBeforeRun() {
	// call init() of the father class
	GA::InitBeforeRun();
	m_self_team_loss_ave.clear();
	m_self_team_loss_best.clear();
	m_enemy_team_loss_ave.clear();
	m_enemy_team_loss_best.clear();
	m_gp << "set title 'Algorithm Status'" << std::endl;
	m_gp << "set xrange [0:" << m_max_generation << "]" << std::endl;
}

Solution<Command> RollingGA::GenerateSolution() {
	//? These is a way to get avaliable abilities in s2client-api, but it is time-consuming
	//? But for now, I choose to limit the chosen of abilities in only move and attack
	size_t team_size = m_my_team.size();
	Solution<Command> sol(team_size, m_objective_size);
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
				}else{
					// hope the last target point is a move position
					action_raw.target_point = sol.variable[i].actions[j-1].target_point + Point2DP(GetRandomFraction() * moveable_radius, GetRandomFraction() * 2 * PI).toPoint2D();
				}
				action_raw.target_point = FixOutsidePointIntoMap(action_raw.target_point, m_game_info.playable_min, m_game_info.playable_max);
			}
			else
			{
				action_raw.ability_id = ABILITY_ID::MOVE;
				action_raw.target_type = ActionRaw::TargetType::TargetPosition;
				// construct move action
				if(j == 0){
					action_raw.target_point = current_location + Point2DP(GetRandomFraction() * moveable_radius, GetRandomFraction() * 2 * PI).toPoint2D();
				}else{
					action_raw.target_point = sol.variable[i].actions[j-1].target_point + Point2DP(GetRandomFraction() * moveable_radius, GetRandomFraction() * 2 * PI).toPoint2D();
				}
				action_raw.target_point = FixOutsidePointIntoMap(action_raw.target_point, m_game_info.playable_min, m_game_info.playable_max);
			}
		}
	}
	return sol;
}

void sc2::RollingGA::Mutate(Solution<Command>& s)
{
	// the user must ensure that the actions is not empty
	ActionRaw& action = GetRandomEntry(GetRandomEntry(s.variable).actions);
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
Population sc2::RollingGA::CrossOver(const Solution<Command> &a, const Solution<Command> &b){
	//todo random select one unit
	int unit_index = GetRandomInteger(0, a.variable.size() - 1);
	//todo exchange the subarray at the same pos in two units' commands
	Population offspring({a, b});
	size_t order_size = a.variable[unit_index].actions.size();
	size_t start = sc2::GetRandomInteger(0, order_size - 1);
	size_t end = sc2::GetRandomInteger(0, order_size - 1);
	if (start > end) {
        std::swap(start, end);
    }
    for (size_t i = start; i < end; i++)
    {
        std::swap(offspring[0].variable[unit_index].actions[i], offspring[1].variable[unit_index].actions[i]);
    }
    return offspring;
}


void sc2::RollingGA::Evaluate(Population& p) {
    assert(p.size() <= m_simulators.size());
    // use different simulators to evaluate solutions respectively
    // for each sim, copy the state and deploy the commands!
    // //todo multi-threaded

	#ifdef MULTI_THREADED
	std::vector<std::thread> setting_threads(m_simulators.size());
    for (size_t i = 0; i < p.size(); i++) {
        setting_threads[i] = std::thread([&, i] {
            m_simulators[i].CopyAndSetState(m_observation, m_is_debug ? &m_debug_renderers[i] : nullptr);
            m_simulators[i].SetOrders(p[i].variable);
        });
    }
    // std::for_each(setting_threads.begin(), setting_threads.end(), [](std::thread& t) -> void { t.join(); });
	int thread_size = setting_threads.size();
	for (size_t i = 0; i < thread_size; i++)
	{
		setting_threads[i].join();
	}
	
	RunSimulatorsSynchronous();
	#else
	for (size_t i = 0; i < p.size(); i++)
	{
		m_simulators[i].CopyAndSetState(m_observation, m_is_debug ? &m_debug_renderers[i] : nullptr);
		m_simulators[i].SetOrders(p[i].variable);
	}
	RunSimulatorsOneByOne();
	#endif
	
    //? output the best one for each generation, or outputs the average objectives for each generation
    float self_loss = 0, self_team_loss_total = 0, self_team_loss_best = std::numeric_limits<float>::max();
    float enemy_loss = 0, enemy_team_loss_total = 0, enemy_team_loss_best = std::numeric_limits<float>::lowest();
    for (size_t i = 0; i < p.size(); i++) {
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

		// 2-objective
        p[i].objectives[0] = m_simulators[i].GetTeamHealthLoss(Unit::Alliance::Enemy);
        p[i].objectives[1] = -m_simulators[i].GetTeamHealthLoss(Unit::Alliance::Self);
    }
    //! output the the results
    std::cout << "ally_team_loss_avg:\t" << self_team_loss_total / p.size() << "\t"
              << "ally_team_loss_best:\t" << self_team_loss_best << "\t"
              << "enemy_team_loss_avg:\t" << enemy_team_loss_total / p.size() << "\t"
              << "enemy_team_loss_best:\t" << enemy_team_loss_best << std::endl;
	//todo store all the data
	m_self_team_loss_ave.push_back(self_team_loss_total / p.size());
	m_self_team_loss_best.push_back(self_team_loss_best);
	m_enemy_team_loss_ave.push_back(enemy_team_loss_total / p.size());
	m_enemy_team_loss_best.push_back(enemy_team_loss_best);
}

void RollingGA::ShowGraphEachGeneration(){
	// set the index here;
	std::vector<float> indices(m_current_generation+1); 
	std::iota(indices.begin(),indices.end(),1);
	// set all the lines to be showed
	m_gp << "set style func linespoints" << std::endl;
	m_gp << "plot" << m_gp.file1d(boost::make_tuple(indices, m_self_team_loss_ave)) << "with lines title 'self lost ave',"
		 << m_gp.file1d(boost::make_tuple(indices, m_self_team_loss_best)) << "with lines title 'self lost best',"
		 << m_gp.file1d(boost::make_tuple(indices, m_enemy_team_loss_ave)) << "with lines title 'enemy lost ave',"
		 << m_gp.file1d(boost::make_tuple(indices, m_enemy_team_loss_best)) << "with lines title 'enemy lost best',"
		 << std::endl;
}

std::vector<const ObservationInterface*> RollingGA::GetAllSimsObservations() const {
    std::vector<const ObservationInterface*> observations(m_simulators.size());
    for (size_t i = 0; i < m_simulators.size(); i++) {
        observations[i] = m_simulators[i].Observation();
    }
    return observations;
}
