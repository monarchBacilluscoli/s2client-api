#ifndef ROLLING_EA_H
#define ROLLING_EA_H

#include "evolutionary_algorithm.h"
#include "../simulator/simulator_pool.h"

namespace sc2
{
class RollingEA : virtual public EvolutionaryAlgorithm<Command> //! because of virtual inheritance, the base class's constructor is invalid
{
protected:
    // game data
    const ObservationInterface *m_observation;
    Units m_my_team;
    GameInfo m_game_info;
    Vector2D m_playable_dis;
    // settings about game
    float m_attack_possibility = 0.7;
    int m_command_length = 8;
    int m_run_length = 300;
    // methods
    ScatterRenderer2D m_objective_distribution;
    DebugRenderers m_debug_renderers;
    // simulators
    SimulatorPool m_simulation_pool;

public:
    RollingEA() = delete;
    RollingEA(const std::string &net_address, int port_start, const std::string &process_path, const std::string &map_path, int max_generation, int population_size, float crossover_rate = .5f, int random_seed = 0) : EvolutionaryAlgorithm(2, max_generation, population_size, random_seed, {std::string("enemy loss"), std::string("my team loss")}), m_debug_renderers(population_size), m_simulation_pool(population_size, net_address, port_start, process_path, map_path)
    {
        m_simulation_pool.StartSimsAsync();
        m_objective_distribution.SetTitle("Objectives Distribution");
        m_objective_distribution.SetXLabel("total damage to enemy");
        m_objective_distribution.SetYLabel("total damage to me");
    }
    virtual ~RollingEA() = default;

    void Initialize(const ObservationInterface *observation);

public:
    // settings about the game
    void SetRunLength(int run_length) { m_run_length = run_length; }
    void SetCommandLength(int command_length) { m_command_length = command_length; }
    void SetAttackPossibility(float attack_possibility) { m_attack_possibility = attack_possibility; }

protected:
    // override functions
    void InitBeforeRun() override;
    void Generate() override;
    void Evaluate() override;
    void Select() override;
    virtual void ShowSolutionDistribution(int showed_generations_count) override;

protected:
    // only belong to this class
    void Evaluate(Population& pop);
    void InitFromObservation();
    void GenerateOne(Solution<Command> &sol);
};

} // namespace sc2

#endif //ROLLING_EA_H