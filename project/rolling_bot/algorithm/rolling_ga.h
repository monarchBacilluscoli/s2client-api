/*! \file rolling_GA.h
    \brief Genetic Algorithm for Rolling Bot to use to optimize the orders for teams.
*/
#ifndef ROLLING_GA
#define ROLLING_GA

#include <sc2api/sc2_api.h>
#include <sc2lib/sc2_utils.h>
#include "../simulator/command.h"
#include "../simulator/simulator.h"
#include "debug_renderer/debug_renderer.h"
#include "ga.h"
#include "solution.h"

namespace sc2{
    class RollingGA :public GA<Command>
    {
    
    public:
        using Population = std::vector<Solution<Command>>;
        using Evaluator = std::function<float(const std::vector<Command>&)>;
        using Compare = std::function<bool(const Solution<Command>&, const Solution<Command>&)>;
        
    public:
        RollingGA() = delete;

        RollingGA(
            const std::string& net_address,
            int port_start,
            const std::string& process_path,
            const std::string& map_path): m_debug_renderers(m_population_size) {
            // simulator number now is simply equal to population size
            m_simulators.resize(m_population_size);
            SetSimulators(net_address, port_start, process_path, map_path);
            // m_debug_renderer.SetIsDisplay(false);
            SetCompare(Solution<Command>::multi_greater);
        }

        // Initialization and setup.
        //? I have to write an initialization function here 
        void InitializeObservation(const ObservationInterface* observation){
            SetObservation(observation);
        }

        void SetRunLength(int run_length);
        void SetCommandLength(int command_length);
        void SetAttackPossibility(float attack_possibility);

        void SetSimlatorsOpponent(const PlayerSetup& opponent);
        void SetSimulatorsMultithreaded(bool multithreaded);

        void SetPopulationSize(int population_size) override {
            if (population_size > m_population_size) {
                m_simulators.resize(population_size);
                // Uses the settings before to set all simulators again
                SetSimulators(m_simulators[0].GetNetAddress(), m_simulators[0].GetPortStart(), m_simulators[0].GetExePath(), m_simulators[0].GetMapPath());
            }
            m_population_size = population_size;
        }
        // Other settings
        void SetDebugMode(bool is_debug);

        // Start up
        void SetSimulatorsStart(const ObservationInterface* observation_interface);
        // Run multiple simulators at the same time (multi-threadedly)
        void RunSimulatorsSynchronous();
        // for test
        void RunSimulatorsOneByOne();
        // Get all the ObservationInterface from those simulators
        std::vector<const ObservationInterface*> GetAllSimsObservations() const;

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
        virtual Solution<Command> GenerateSolution() override;
        //! According to game conditions generates solutions which is as valid as possiable
        virtual void Mutate(Solution<Command>& s) override;
        //! Plaese set the start point before you evaluate
        virtual void Evaluate(Population& p) override;
        // to take place the default function
        //virtual void EvaluateSingleSolution(Solution<Command>& s) override {};

        // Settings
        int m_objective_size = 2;
        int m_sims_step_size = 1;
        int m_run_length = 128;  //? Does the step_size in simulator matter?
        //! the command length for each unit
        int m_command_length = 8;
        float m_attack_possibility = 0.3f; // it's related to the m_run_length
        int m_mutate_step = 100;
        //! to reduce uncertainty, uses multiple simulators to evaluate one solution
        //! this is the number of sims to be used to evaluate one solution
        int m_evaluate_multiplier = 3;
        bool m_is_debug = false;

        // Information
        //? if I have many simulators here, things will become a non-blocking multi-thread programming condition, that means I need to manage them to function correctly
        std::vector<Simulator> m_simulators;
        // map info to privide bundary of the map (maybe useless)
        GameInfo m_game_info;
        Point2D m_playable_dis;
        //! unit type info
        UnitTypes m_unit_type;
        //! current number of my units
        Units m_my_team;
        Units m_enemy_team;
        const ObservationInterface* m_observation;

        // Tools
        DebugRenderers m_debug_renderers;

        // Misc
        
        const double PI = atan(1.) * 4.;
    };
}

#endif // !ROLLING_GA
