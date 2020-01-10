#ifndef ROLLING_EA_H
#define ROLLING_EA_H

#include "global_defines.h"

#include "evolutionary_algorithm.h"
#include "../simulator/simulator_pool.h"
#include "rolling_solution.h"

namespace sc2
{
class RollingEA : virtual public EvolutionaryAlgorithm<Command, RollingSolution> //! because of virtual inheritance, the base class's constructor is invalid
{
public:
    using EA = EvolutionaryAlgorithm<Command, RollingSolution>;

public:
    class ConvergenceTermination // convergence termination condition checker for this class
    {
    private:
        // settings
        const RollingEA &m_algo;
        float m_no_improve_threshold = .01f; //
        int m_max_no_impreve_generation = 20;
        // data
        int m_current_no_improve_generation = 0;
        std::vector<float> m_last_record_obj_average;

    public:
        ConvergenceTermination(const RollingEA &algo) : m_algo(algo){};
        ConvergenceTermination(const RollingEA &algo, int max_no_improve_generation, float no_improve_tolerance) : m_algo(algo), m_max_no_impreve_generation(max_no_improve_generation), m_no_improve_threshold(no_improve_tolerance){};
        ~ConvergenceTermination() = default;
        bool operator()();

        float GetNoImproveTolerance() { return m_no_improve_threshold; };
        int GetMaxNoImproveGeneration() { return m_max_no_impreve_generation; };
        void SetNoImproveTolerance(float no_improve_tolerance) { m_no_improve_threshold = no_improve_tolerance; };
        void SetMaxNoInproveGeneration(int max_no_improve_generation) { m_max_no_impreve_generation = max_no_improve_generation; };
        void clear(); // for the use of next run
    };

    struct memory
    {
        std::map<Tag, Command> best_iron_commands;
        std::map<Tag, Command> best_normal_commands;
        std::map<Tag, Command> best_run_commands;
    };

protected:
    // game data
    const ObservationInterface *m_observation;
    Units m_my_team; // updated at the beginning of each algorithm run
    Units m_enemy_team; // updated at the beginning of each algorithm run
    GameInfo m_game_info;
    Vector2D m_playable_dis;
    UnitTypes m_unit_types; // metadata of units. Array can be indexed directly by UnitID (Unit->unit_type).
    // settings about game
    float m_attack_possibility = 0.7;
    unsigned int m_command_length = 8;
    unsigned int m_sim_length = 300;
    unsigned int m_evaluation_time_multiplier = 1; // the evaluation times for each solution (to avoid randomness)
#ifdef USE_GRAPHICS
    ScatterRenderer2D m_objective_distribution;
    DebugRenderers m_debug_renderers;
#endif //USE_GRAPHICS
    bool m_use_fix = false;
    bool m_use_priori = false;
    bool m_use_assemble = false;
    bool m_is_debug = true;
    // simulators
    SimulatorPool m_simulation_pool;
    // methods
    ConvergenceTermination m_convergence_termination_manager{*this};
    std::lognormal_distribution<float> m_log_dis{0, 0.6};

public:
    RollingEA() = delete;
    RollingEA(const std::string &net_address, int port_start, const std::string &process_path, const std::string &map_path, int max_generation, int population_size, int random_seed = 0, unsigned int evaluation_time_multiplier = 1) : EvolutionaryAlgorithm(2, max_generation, population_size, random_seed, {std::string("enemy loss"), std::string("my team loss")}),
#ifdef USE_GRAPHICS
                                                                                                                                                                                                                                         m_debug_renderers(population_size),
#endif //USE_GRAPHICS
                                                                                                                                                                                                                                         m_simulation_pool(population_size,
                                                                                                                                                                                                                                                           net_address,
                                                                                                                                                                                                                                                           port_start,
                                                                                                                                                                                                                                                           process_path,
                                                                                                                                                                                                                                                           map_path),
                                                                                                                                                                                                                                         m_evaluation_time_multiplier(evaluation_time_multiplier)
    {
        m_termination_conditions[TERMINATION_CONDITION::CONVERGENCE] = std::ref(m_convergence_termination_manager);
        m_simulation_pool.StartSimsAsync();
#ifdef USE_GRAPHICS
        m_objective_distribution.SetTitle("Objectives Distribution");
        m_objective_distribution.SetXLabel("total damage to enemy");
        m_objective_distribution.SetYLabel("total damage to me");
#endif
        SetObjectiveNames({"Enemy Loss", "My Team Loss"});
        for (auto &sol : m_population)
        {
            sol.results.resize(evaluation_time_multiplier);
        }
    }
    virtual ~RollingEA() = default;

    void Initialize(const ObservationInterface *observation);

public:
    // settings about the game
    void SetSimLength(unsigned int sim_length) { m_sim_length = sim_length; }
    void SetCommandLength(unsigned int command_length) { m_command_length = command_length; }
    void SetAttackPossibility(float attack_possibility) { m_attack_possibility = attack_possibility; }
    void SetEvaluationTimeMultiplier(unsigned int times)
    {
        m_evaluation_time_multiplier = times;
        for (auto &sol : m_population)
        {
            sol.results.resize(m_evaluation_time_multiplier);
        }
    }
    void SetUseFix(bool use_fix) { m_use_fix = use_fix; };
    void SetUsePriori(bool use_priori) { m_use_priori = use_priori; };
    void SetUseAssemble(bool use_assemble) {m_use_assemble = use_assemble;};
    void SetDebug(bool is_debug) { m_is_debug = is_debug; }

protected:
    // override functions
    virtual void InitBeforeRun() override;
    void InitOnlySelfMembersBeforeRun();
    void Generate() override;
    void Evaluate() override;
    void Select() override; // sort and keep only the good solutions
#ifdef USE_GRAPHICS
    virtual void ShowOverallStatusGraphEachGeneration() override;
    virtual void ShowSolutionDistribution(int showed_generations_count) override;
#endif //USE_GRAPHICS
    //! for test
    virtual void ActionAfterEachGeneration() override
    {
        EA::ActionAfterEachGeneration();
        //todo another implementation: compound some solutions -> evaluate them -> sort -> put them into solutions
    };

protected:
    // only belong to this class
    void Evaluate(Population &pop);
    void InitFromObservation();
    void GenerateOne(RollingSolution<Command> &sol);
    virtual void RecordObjectives() override;
    virtual void ActionAfterRun() override;
    RollingSolution<Command> AssembleASolutionFromGoodUnits(const Population& evaluated_pop); // Assemble a priori solution based on the evaluated population.
    void AssembleASolutionFromGoodUnits(RollingSolution<Command>& modified_solution ,const Population &evaluated_pop); //! the param must be an evaluated population
    RollingSolution<Command> AssembleASolutionFromGoodUnits(const std::vector<RollingSolution<Command>*>& evaluated_pop); // Assemble a priori solution based on the evaluated population. //! the param must be an evaluated population

protected: // some utilities
    Point2D FixActionPosIntoEffectiveRangeToNearestEnemy(const Point2D &action_target_pos, float this_unit_weapon_range, const Units &enemy_team);
};

} // namespace sc2

#endif //ROLLING_EA_H