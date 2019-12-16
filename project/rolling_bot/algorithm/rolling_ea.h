#ifndef ROLLING_EA_H
#define ROLLING_EA_H

#include "global_defines.h"

#include "evolutionary_algorithm.h"
#include "../simulator/simulator_pool.h"

namespace sc2
{
class RollingEA : virtual public EvolutionaryAlgorithm<Command> //! because of virtual inheritance, the base class's constructor is invalid
{
public:
    using EA = EvolutionaryAlgorithm<Command>;

protected:
    // game data
    const ObservationInterface *m_observation;
    Units m_my_team;
    Units m_enemy_team; // updated at the beginning of each run
    GameInfo m_game_info;
    Vector2D m_playable_dis;
    UnitTypes m_unit_types; // metadata of units. Array can be indexed directly by UnitID (Unit->unit_type).
    
    // settings about game
    float m_attack_possibility = 0.7;
    int m_command_length = 8;
    int m_sim_length = 300;
    int m_evaluation_time_multiplier = 1; // the evaluation times for each solution (to avoid randomness)
    // methods
#ifdef USE_GRAPHICS
    ScatterRenderer2D m_objective_distribution;
    DebugRenderers m_debug_renderers;
#endif //USE_GRAPHICS
    bool m_is_debug = true;
    // simulators
    SimulatorPool m_simulation_pool;

public:
    RollingEA() = delete;
    RollingEA(const std::string &net_address, int port_start, const std::string &process_path, const std::string &map_path, int max_generation, int population_size, int random_seed = 0) : EvolutionaryAlgorithm(2, max_generation, population_size, random_seed, {std::string("enemy loss"), std::string("my team loss")}),
#ifdef USE_GRAPHICS
                                                                                                                                                                                            m_debug_renderers(population_size),
#endif //USE_GRAPHICS
                                                                                                                                                                                            m_simulation_pool(population_size,
                                                                                                                                                                                                              net_address,
                                                                                                                                                                                                              port_start,
                                                                                                                                                                                                              process_path,
                                                                                                                                                                                                              map_path)
    {
        m_simulation_pool.StartSimsAsync();
#ifdef USE_GRAPHICS
        m_objective_distribution.SetTitle("Objectives Distribution");
        m_objective_distribution.SetXLabel("total damage to enemy");
        m_objective_distribution.SetYLabel("total damage to me");
#endif
        SetObjectiveNames({"Enemy Loss", "My Team Loss"});
    }
    virtual ~RollingEA() = default;

    void Initialize(const ObservationInterface *observation);

public:
    // settings about the game
    void SetSimLength(int sim_length) { m_sim_length = sim_length; }
    void SetCommandLength(int command_length) { m_command_length = command_length; }
    void SetAttackPossibility(float attack_possibility) { m_attack_possibility = attack_possibility; }
    void SetEvaluationTimeMultiplier(int times) { m_evaluation_time_multiplier = times; }
    void SetDebug(bool is_debug) { m_is_debug = is_debug; }

protected:
    // override functions
    virtual void InitBeforeRun() override;
    void InitOnlySelfMembersBeforeRun();
    void Generate() override;
    void Evaluate() override;
    void Select() override;
#ifdef USE_GRAPHICS
    virtual void ShowOverallStatusGraphEachGeneration() override;
    virtual void ShowSolutionDistribution(int showed_generations_count) override;
#endif //USE_GRAPHICS
    //! for test
    virtual void ActionAfterEachGeneration() override
    {
        EA::ActionAfterEachGeneration();
    };

protected:
    // only belong to this class
    void Evaluate(Population &pop);
    void InitFromObservation();
    void GenerateOne(Solution<Command> &sol);
    virtual void RecordObjectives() override;
    virtual void ActionAfterRun() override;
};

} // namespace sc2

#endif //ROLLING_EA_H