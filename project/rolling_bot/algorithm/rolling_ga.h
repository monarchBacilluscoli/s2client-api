/*! \file rolling_GA.h
    \brief Genetic Algorithm for Rolling Bot to use to optimize the orders for teams.
*/
#ifndef ROLLING_GA_H
#define ROLLING_GA_H

#include <sc2api/sc2_api.h>
#include <sc2lib/sc2_utils.h>
#include "rolling_ea.h"
#include "genetic_algorithm.h"

namespace sc2
{
class RollingGA : public RollingEA, public GeneticAlgorithm<Command>
{
protected:
    int m_mutate_step = 100; //?

    // Misc
    const double PI = atan(1.) * 4.;

public:
    using Population = std::vector<Solution<Command>>;
    using GA = GeneticAlgorithm<Command>;
    using EA = EvolutionaryAlgorithm<Command>;

public:
    RollingGA() = delete;

    RollingGA(
        const std::string &net_address,
        int port_start,
        const std::string &process_path,
        const std::string &map_path,
        int max_generation = 50,
        int population_size = 50,
        float crossover_rate = 1.f,
        float mutation_rate = 0.f,
        int random_seed = rand()) : EA(2, max_generation, population_size, random_seed, {"Enemy Loss", "My Team Loss"}), GA(2, max_generation, population_size, crossover_rate, mutation_rate, random_seed), RollingEA(net_address, port_start, process_path, map_path, max_generation, population_size, random_seed)
    {
    }

    ~RollingGA() = default;

protected:
    virtual void InitBeforeRun() override;
    virtual void InitOnlySelfMemeberBeforeRun();
    virtual void Mutate() override;
    virtual void Crossover() override;

    Population Crossover_(const Solution<Command> &a, const Solution<Command> &b);
    void Mutate_(Solution<Command> &a);
};
} // namespace sc2

#endif // !ROLLING_GA_H
