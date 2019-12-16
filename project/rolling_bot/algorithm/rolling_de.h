#ifndef ROLLING_DE_H
#define ROLLING_DE_H

#include "differential_evolution.h"
#include "../simulator/simulator_pool.h"
#include "gnuplot-iostream.h"
#include "rolling_ea.h"

namespace sc2
{

class RollingDE : public DifferentialEvolution<Command>, public RollingEA
{
protected:
public:
    RollingDE() = delete;
    /* I must call the construct of the virtual base class, since all the virtual derived classes will not call it (since the multiple calls will cause multiple constructing, it is banned)
    So, the parameters in constructors of the direct virtual derived classes are useless.
    */
    RollingDE(const std::string &net_address,
              int port_start,
              const std::string &process_path,
              const std::string &map_path,
              int max_generation = 50,
              int population_size = 50,
              float scale_factor = .5f,
              float crossover_rate = .5f,
              int random_seed = rand()) : EvolutionaryAlgorithm(2, max_generation, population_size, random_seed, {"Enemy Loss", "My Team Loss"}), DifferentialEvolution(2, max_generation, population_size, scale_factor, crossover_rate), RollingEA(net_address, port_start, process_path, map_path, max_generation, population_size, random_seed)
    {
        SetAttackPossibility(.9f);
    }
    virtual ~RollingDE() = default;

protected:
    void InitBeforeRun() override;
    virtual Solution<Command> Mutate(const Solution<Command> &base_sol, const Solution<Command> &material_sol1, const Solution<Command> &material_sol2) override;
    virtual void Crossover(const Solution<Command> &parent, Solution<Command> &child) override;
};
} // namespace sc2

#endif //ROLLING_DE_H