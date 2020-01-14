#ifndef GENETIC_ALGORITHM_H
#define GENETIC_ALGORITHM_H

#include "evolutionary_algorithm.h"

namespace sc2
{
template <typename T, template <typename> typename TSolution>
class GeneticAlgorithm : virtual public EvolutionaryAlgorithm<T, TSolution>
{
    using EA = EvolutionaryAlgorithm<T, TSolution>;

protected:
    //settings
    float m_crossover_rate = 1.f;
    float m_mutation_rate = .3f;

public:
    GeneticAlgorithm() : EvolutionaryAlgorithm<T, TSolution>(){};
    GeneticAlgorithm(int objective_size,
                     int max_generation,
                     int population_size, float crossover_rate = 1.f, float mutation_rate = .3f, int random_seed = 0, std::vector<std::string> objective_names = std::vector<std::string>()) : EA(objective_size, max_generation, population_size, random_seed, objective_names), m_crossover_rate(crossover_rate), m_mutation_rate(mutation_rate){};
    ~GeneticAlgorithm() = default;

    void SetCrossoverRate(float rate)
    {
        m_crossover_rate = rate;
    }
    float CrossoverRate()
    {
        return m_crossover_rate;
    }
    void SetMutationRate(float rate)
    {
        m_mutation_rate = rate;
    }
    float MutationRate()
    {
        return m_mutation_rate;
    }

protected:
    virtual void Breed() override;

    virtual void InitBeforeRun() override;
    void InitOnlySelfMemeberBeforeRun();
    virtual void Crossover() = 0;
    virtual void Mutate() = 0;
};

template <typename T, template <typename> class TSolution>
void GeneticAlgorithm<T, TSolution>::InitBeforeRun()
{
    EA::InitBeforeRun();
    InitOnlySelfMemeberBeforeRun();
    //todo something else?
}

template <typename T, template <typename> class TSolution>
void GeneticAlgorithm<T, TSolution>::Breed()
{
    Crossover();
    Mutate();
}

template <typename T, template <typename> class TSolution>
void GeneticAlgorithm<T, TSolution>::InitOnlySelfMemeberBeforeRun()
{
    //nothing
}

} // namespace sc2

#endif //GENETIC_ALGORITHM_H