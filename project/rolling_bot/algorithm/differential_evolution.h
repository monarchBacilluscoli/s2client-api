#ifndef DIFFERENTIAL_EVOLUTION_H
#define DIFFERENTIAL_EVOLUTION_H

#include <algorithm>
#include "evolutionary_algorithm.h"

namespace sc2
{
template <class T>
class DifferentialEvolution : virtual public EvolutionaryAlgorithm<T>
{
    using EA = EvolutionaryAlgorithm<T>;

protected:
    // settings
    float m_scale_factor = .5f;
    float m_crossover_rate = .5f;

    // methods
    // std::vector<int> m_index_vec{}; // used for get unduplicated random numbers

public:
    DifferentialEvolution() : EvolutionaryAlgorithm<T>(){};
    //todo constructor with all parameters
    DifferentialEvolution(int objective_size,
                          int max_generation,
                          int population_size,
                          float scale_factor = .5,
                          float crossover_rate = .5f, int random_seed = 0) : EvolutionaryAlgorithm<T>(objective_size, max_generation, population_size, random_seed), m_scale_factor(scale_factor), m_crossover_rate(crossover_rate){};
    virtual ~DifferentialEvolution() = default;

    void SetCrossoverRate(float rate)
    {
        m_crossover_rate = rate;
    }
    float CrossoverRate()
    {
        return m_crossover_rate;
    }
    void SetScaleFactor(float factor)
    {
        m_scale_factor = factor;
    }
    float ScaleFactor()
    {
        return m_scale_factor;
    }

protected:
    virtual void InitBeforeRun() override;
    void InitOnlySelfMembersBeforeRun();
    virtual void Breed() override;

    virtual Solution<T> Mutate(const Solution<T> &base_sol, const Solution<T> &material_sol1, const Solution<T> &material_sol2) = 0;
    virtual void Crossover(const Solution<T> &parent, Solution<T> &child) = 0; // can not be implemented, since there are so many solution types
};

template <class T>
void DifferentialEvolution<T>::InitBeforeRun()
{
    EvolutionaryAlgorithm<T>::InitBeforeRun();
    //todo the code only belonging to DE<T>
}

template <class T>
void DifferentialEvolution<T>::InitOnlySelfMembersBeforeRun() {}

template <class T>
void DifferentialEvolution<T>::Breed()
{
    // mutate each? solution in population, get the transition solution
    int sz = DifferentialEvolution<T>::m_population.size();
    EA::m_offspring.resize(sz, Solution<T>(0, EA::m_objective_size));
    std::uniform_int_distribution<int> random_dis(0, sz - 1);
    for (size_t i = 0; i < sz; ++i)
    {
        //? Here I can not ensure the 3 random numbers are not the same. Should I ensure it?
        int index_a = random_dis(EA::m_random_engine);
        int index_b = random_dis(EA::m_random_engine);
        Solution<T> &material_a = EA::m_population[index_a];
        Solution<T> &material_b = EA::m_population[index_b];
        EA::m_offspring[i] = Mutate(EA::m_population[i], material_a, material_b);
        Crossover(EA::m_population[i], EvolutionaryAlgorithm<T>::m_offspring[i]);
    }
}

} // namespace sc2

#endif // DIFFERENTIAL_EVOLUTION_H