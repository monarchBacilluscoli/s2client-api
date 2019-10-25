#ifndef ROLLING_DE_H
#define ROLLING_DE_H

#include "differential_evolution.h"
#include "gnuplot-iostream.h"
#include "../simulator/simulator_pool.h"
#include "../methods/graph_renderer.h"
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
    RollingDE(const std::string &net_address, int port_start, const std::string &process_path, const std::string &map_path, int objective_size, int max_generation, int population_size, int crossover_rate = .5f, int random_seed = 0) : EvolutionaryAlgorithm(objective_size, max_generation, population_size, random_seed), DifferentialEvolution(objective_size, max_generation, population_size, crossover_rate, random_seed), RollingEA(net_address, port_start, process_path, map_path, objective_size, max_generation, population_size, crossover_rate, random_seed)
    {
    }
    virtual ~RollingDE();

protected:
};
} // namespace sc2

#endif //ROLLING_DE_H