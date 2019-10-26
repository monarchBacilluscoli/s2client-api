#ifndef ROLLING_EA_H
#define ROLLING_EA_H

#include "evolutionary_algorithm.h"
#include "../simulator/simulator_pool.h"

namespace sc2
{
class RollingEA : virtual public EvolutionaryAlgorithm<Command>
{
protected:
    // game data

    // methods
    ScatterRenderer2D m_objective_distribution;
    // simulators
    SimulatorPool m_simulator_pool;

public:
    RollingEA() = delete;
    RollingEA(const std::string &net_address, int port_start, const std::string &process_path, const std::string &map_path, int max_generation, int population_size, int crossover_rate = .5f, int random_seed = 0) : EvolutionaryAlgorithm(2, max_generation, population_size, random_seed, {std::string("enemy loss"), std::string("my team loss")}), m_simulator_pool(population_size, net_address, port_start, process_path, map_path)
    {
        m_simulator_pool.StartSimsAsync();
        m_objective_distribution.SetTitle("Objectives Distribution");
        m_objective_distribution.SetXLabel("total damage to enemy");
        m_objective_distribution.SetYLabel("total damage to me");
    }
    virtual ~RollingEA();

    void Initialize(const ObservationInterface *observation);

protected:
    void InitializeFromObservation(const ObservationInterface *observation);
    virtual void ShowGraphEachGeneration() override;
};

} // namespace sc2

#endif //ROLLING_EA_H