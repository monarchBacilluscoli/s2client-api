/*! \file rolling_GA.h
    \brief Genetic Algorithm for Rolling Bot to use to optimize the orders for teams.

    Here should be a discription of this class's functions
*/
#ifndef ROLLING_GA
#define ROLLING_GA

#include<sc2api/sc2_api.h>
#include "../Algorithm/GA.h"
#include "../command.h"
#include "simulator.h"
#include "../utilities/sc2utility.h"
#include "../utilities/Point2DPolar.h"

namespace sc2 {
    class RollingGA :public GA<Command>
    {

        using Population = std::vector<Solution<Command>>;
        using Evaluator = std::function<float(const std::vector<Command>&)>;
        using Compare = std::function<bool(const Solution<Command>&, const Solution<Command>&)>;

    public:
        RollingGA() = default;
        RollingGA(\
            int step_size, \
            Simulator& simulator, \
            const ObservationInterface* observation, \
            std::vector<Evaluator>& evaluators, \
            float crossover_rate = 1.f, \
            int population_size = 100, \
            Compare compare = Solution<Command>::sum_greater, \
            int max_generation = 100, \
            float reproduce_rate = 1.f, \
            float mutate_rate = 0.3f \
        ) :GA<Command>(evaluators, crossover_rate, mutate_rate, population_size, compare, max_generation, reproduce_rate), m_step_size(step_size), m_simulator(simulator), m_observation(observation) {
            //? I can not get those object before game starting
            m_game_info = observation->GetGameInfo();
            m_unit_type = observation->GetUnitTypeData(); //? why isn't here anything wrong?
            m_my_team = m_observation->GetUnits(Unit::Alliance::Self);
            m_enemy_team = m_observation->GetUnits(Unit::Alliance::Enemy);
            m_playable_dis = Point2D(m_game_info.playable_max.x - m_game_info.playable_min.x, m_game_info.playable_max.y - m_game_info.playable_min.y);
        }
        //! before every time you run it, you should do it once
        void Initialize(\
            int step_size, \
            Simulator& simulator, \
            const ObservationInterface* observation,\
            std::vector<Evaluator>& evaluators, \
            float crossover_rate = 1.f, \
            int population_size = 100, \
            Compare compare = Solution<Command>::sum_greater, \
            int max_generation = 100, \
            float reproduce_rate = 1.f, \
            float mutate_rate = 0.3f) {
            GA::Initialize(evaluators, crossover_rate, mutate_rate, population_size, compare, max_generation, reproduce_rate);
            m_game_info = observation->GetGameInfo();
            m_unit_type = observation->GetUnitTypeData(); //? why isn't here anything wrong?
            m_my_team = m_observation->GetUnits(Unit::Alliance::Self);
            m_enemy_team = m_observation->GetUnits(Unit::Alliance::Enemy);
            m_playable_dis = Point2D(m_game_info.playable_max.x - m_game_info.playable_min.x, m_game_info.playable_max.y - m_game_info.playable_min.y);
        }

        ~RollingGA() = default;
    private:
        //! According to known information generates solutions which is as valid as possiable 
        virtual Solution<Command> GenerateSolution() override;;
        //! According to game conditions generates solutions which is as valid as possiable
        virtual void Mutate(Solution<Command>& s) override;
        //! Settings
        int m_step_size = 8; //? Does the step_size in simulator matter?
        //! the command length for each unit
        int m_command_size = 8;
        float m_attack_possibility = 0.3f; // it's related to the m_step_size
        int m_mutate_step = 100;

        //! Information
        //? if I have many simulators here, things will become a non-blocking multi-thread programming condition, that means I need to manage them to function correctly
        Simulator& m_simulator = Simulator(); //? Just one simulator here? Or I need to pass a simulator here
        // map info to privide bundary of the map (maybe useless)
        GameInfo m_game_info;
        Point2D m_playable_dis;
        //todo unit type info
        UnitTypes m_unit_type;
        //! current number of my units
        //int m_team_size;
        Units m_my_team;
        Units m_enemy_team;
        const ObservationInterface* m_observation;

        const double PI = atan(1.) * 4.;
    };
}

#endif // !ROLLING_GA

