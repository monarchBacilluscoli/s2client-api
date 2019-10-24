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
#include "gnuplot-iostream.h"
#include "../simulator/simulator_pool.h"
#include "../methods/graph_renderer.h"

namespace sc2
{
class RollingGA : public GA<Command>
{

public:
    using Population = std::vector<Solution<Command>>;
    using Evaluator = std::function<float(const std::vector<Command> &)>;
    using Compare = std::function<bool(const Solution<Command> &, const Solution<Command> &)>;

public:
    RollingGA() = delete;

    RollingGA(
        const std::string &net_address,
        int port_start,
        const std::string &process_path,
        const std::string &map_path,
        size_t population_size = 50) : GA::GA(population_size), m_debug_renderers(population_size), m_simulation_pool(population_size, net_address, port_start, process_path, map_path)
    {
        m_simulation_pool.StartSimsAsync();
        m_objectives_distribution_graph.SetTitle("Solution Distribution");
        m_objectives_distribution_graph.SetXLabel("total damage to enemy");
        m_objectives_distribution_graph.SetYLabel("total damage to me");
        // m_debug_renderer.SetIsDisplay(false);
        // SetCompare(Solution<Command>::multi_greater);
    }

    // Initialization and setup.
    //? I have to write an initialization function here
    void Initialize(const ObservationInterface *observation)
    {
        SetInfoFromObservation(observation);
    }

    void SetRunLength(int run_length);
    void SetCommandLength(int command_length);
    void SetAttackPossibility(float attack_possibility);

    // void SetSimlatorsOpponent(const PlayerSetup& opponent);
    // void SetSimulatorsMultithreaded(bool multithreaded);

    // //? here is something wrong
    // void SetPopulationSize(int population_size) override {
    //     if (population_size > m_population_size) {
    //         m_simulators.resize(population_size);
    //         // Uses the settings before to set all simulators again
    //         SetSimulators(m_simulators[0].GetNetAddress(), m_simulators[0].GetPortStart(), m_simulators[0].GetExePath(), m_simulators[0].GetMapPath());
    //     }
    //     m_population_size = population_size;
    // }
    // Other settings
    void SetDebugMode(bool is_debug);

    // // Start up
    // void SetSimulatorsStart(const ObservationInterface* observation_interface);
    // Run multiple simulators at the same time (multi-threadedly)
    // void RunSimulatorsSynchronous();
    // for test
    // void RunSimulatorsOneByOne();
    // Get all the ObservationInterface from those simulators
    // std::vector<const ObservationInterface*> GetAllSimsObservations() const;

    ~RollingGA() = default;

private:
    void SetInfoFromObservation(const ObservationInterface *observation);
    //! Use this to set the the client to run simulators, the map loaded and other settings
    // void SetSimulators(const std::string& net_address,
    //     int port_start,
    //     const std::string& process_path,
    //     const std::string& map_path
    // );

    // run
    //!
    virtual void InitBeforeRun() override;
    //! According to known information generates solutions which is as valid as possiable
    virtual Solution<Command> GenerateSolution() override;
    //!
    virtual void Mutate(Solution<Command> &s) override;
    //! choose only one unit and exchange a subarray of orders of each solution
    virtual Population CrossOver(const Solution<Command> &s1, const Solution<Command> &s2) override;
    //! Plaese set the start point before you evaluate
    virtual void Evaluate(Population &p) override;
    // to take place the default function
    //virtual void EvaluateSingleSolution(Solution<Command>& s) override {};
    virtual void ShowGraphEachGeneration() override;
    // // divide the population by rank into subsets which size is set by users
    // static void DividePopulationByRank(const Population &pop, std::vector<std::vector<Solution<Command>>> &subsets)
    // {
    //     if (pop.empty() || pop[0].rank == std::numeric_limits<int>::max())
    //     {
    //         return;
    //     }
    //     int sz = subsets.size();
    //     for (size_t i = 0; i < sz; i++)
    //     {

    //     }
    // }
    virtual void SortSolutions(Population &pop) override
    {
        DominanceSort(pop);
    }

    // Settings
    int m_objective_size = 2;
    int m_sims_step_size = 1;
    int m_run_length = 300;
    //! the command length for each unit
    int m_command_length = 8;
    float m_attack_possibility = 0.9f; // it's related to the m_run_length
    int m_mutate_step = 100;
    //! to reduce uncertainty, uses multiple simulators to evaluate one solution
    //! this is the number of sims to be used to evaluate one solution
    int m_evaluate_multiplier = 3;
    bool m_is_debug = false;

    // thread pool (used to put aside those thread without answer)
    SimulatorPool m_simulation_pool;

    // Information
    // map info to privide bundary of the map (maybe useless)
    GameInfo m_game_info;
    Point2D m_playable_dis;
    //! unit type info
    UnitTypes m_unit_type;
    //! current number of my units
    Units m_my_team;
    Units m_enemy_team;
    // for easy use, set in Initialize()
    const ObservationInterface *m_observation;

    // Information of generations, needed to show the algorithm status
    std::vector<float> m_self_team_loss_ave;
    std::vector<float> m_self_team_loss_best;
    std::vector<float> m_enemy_team_loss_ave;
    std::vector<float> m_enemy_team_loss_best;
    std::vector<std::vector<float>> m_last_solution_dis{};

    // Tools
    DebugRenderers m_debug_renderers;
    LineChartRenderer2D m_line_chart_renderer;
    ScatterRenderer2D m_objectives_distribution_graph;

    // Misc
    const double PI = atan(1.) * 4.;
};
} // namespace sc2

#endif // !ROLLING_GA
