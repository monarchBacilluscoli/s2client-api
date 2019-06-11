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
        RollingGA() = delete;
        //? the constructor only includes those parameters that must be set, other params can be set respectively

        // This 
        RollingGA(
            const std::string& net_address,
            int port_start,
            const std::string& process_path,
            const std::string& map_path) {
            m_simulators.resize(m_population_size);
            SetSimulators(net_address, port_start, process_path, map_path);
        }

        // Initialization and setup.
        //? I have to write an initialization function here 
        void InitializeObservation(const ObservationInterface* observation){
            SetObservation(observation);
        }

        void SetStepSize(int step_size);
        void SetCommandLength(int command_length);
        void SetAttackPossibility(float attack_possibility);

        void SetSimlatorsOpponent(const PlayerSetup& opponent);
        void SetSimulatorsMultithreaded(bool multithreaded);

        void SetPopulationSize(int population_size) override {
            if (population_size > m_population_size) {
                m_simulators.resize(population_size);
                // Uses the settings before to set all simulators again
                SetSimulators(m_simulators[0].GetNetAddress(), m_simulators[0].GetPortStart(), m_simulators[0].GetProcessPath(), m_simulators[0].GetMapPath());
            }
            m_population_size = population_size;
        }

        // Start up
        void SetSimulatorsStart(const ObservationInterface* observation_interface);
        void RunSimulatorsSynchronous();

        ~RollingGA() = default;
    private:
        void SetObservation(const ObservationInterface* observation);
        //! Use this to set the the client to run simulators, the map loaded and other settings
        void SetSimulators(const std::string& net_address,
            int port_start,
            const std::string& process_path,
            const std::string& map_path
        );

        // run
        //! According to known information generates solutions which is as valid as possiable 
        virtual Solution<Command> GenerateSolution() override;;
        //! According to game conditions generates solutions which is as valid as possiable
        virtual void Mutate(Solution<Command>& s) override;
        //! Plaese set the start point before you evaluate
        virtual void Evaluate(Population& p) override;
        // to take place the default function
        //virtual void EvaluateSingleSolution(Solution<Command>& s) override {};

        // Settings
        int m_step_size = 8; //? Does the step_size in simulator matter?
        //! the command length for each unit
        int m_command_length = 8;
        float m_attack_possibility = 0.3f; // it's related to the m_step_size
        int m_mutate_step = 100;
        //! to reduce uncertainty, uses multiple simulators to evaluate one solution
        //! this is the number of sims to be used to evaluate one solution
        int m_evaluate_multiplier = 3;

        // Information
        //? if I have many simulators here, things will become a non-blocking multi-thread programming condition, that means I need to manage them to function correctly
        std::vector<Simulator> m_simulators;
        // map info to privide bundary of the map (maybe useless)
        GameInfo m_game_info;
        Point2D m_playable_dis;
        //! unit type info
        static UnitTypes m_unit_type;
        //! current number of my units
        Units m_my_team;
        Units m_enemy_team;
        const ObservationInterface* m_observation;

        const double PI = atan(1.) * 4.;
    };

}


#endif // !ROLLING_GA

