#include "rolling_bot.h"

namespace sc2 {
	void rolling_bot::OnGameStart()
	{
		// only after game starting I can initialize the ga, or the information will not be passed to it
		m_rolling_ga.Initialize(8, m_sim, Observation(), evaluators);
	}
	void rolling_bot::OnStep()
	{
		// after a specific interval, the algorhim should run once
		if (Observation()->GetGameLoop() % m_interval_size == 0) {
			//todo first setup the simulator
			m_sim.CopyAndSetState(Observation());
			//todo then pass it to algorithm and let algorithm run
			Solution<Command> sol = m_rolling_ga.Run().front(); // you must control the frames to run in m_sim.Initialize(), not here
			//todo after running, get the solution to deploy
			DeploySolution(sol);
		}
	}
}

