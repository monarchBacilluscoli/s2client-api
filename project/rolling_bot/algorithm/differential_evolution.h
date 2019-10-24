#ifndef DIFFERENTIAL_EVOLUTION
#define DIFFERENTIAL_EVOLUTION

#include <algorithm>
#include "evolutionary_algorithm.h"

namespace sc2
{
template <class T>
class DifferentialEvolution : public EvolutionaryAlgorithm<T>
{
    using EA = EvolutionaryAlgorithm<T>;

protected:
    // settings
    float m_crossover_rate = .5f;

    // methods
    std::vector<int> m_index_vec{}; // used for get unduplicated random numbers

public:
    DifferentialEvolution() : EvolutionaryAlgorithm<T>(){};
    //todo constructor with all parameters
    DifferentialEvolution(int max_generation,
                          int population_size,
                          int objective_size,
                          int crossover_rate = .5f, int random_seed = 0) : EvolutionaryAlgorithm<T>(max_generation, population_size, objective_size, random_seed), m_crossover_rate{crossover_rate}, m_index_vec{population_size}
    {
        std::iota(m_index_vec.begin(), m_index_vec.end(), 0);
    };
    virtual ~DifferentialEvolution() = default;

protected:
    virtual void InitBeforeRun() override;
    virtual void Breed() override;

    virtual Solution<T> Mutate(const Solution<T> &base_sol, const Solution<T> &material_sol1, const Solution<T> &material_sol2) = 0;
    virtual Solution<T> Crossover(const Solution<T> &parent, const Solution<T> &child) = 0; // can not be implemented, since there are so many solution types
};

template <class T>
void DifferentialEvolution<T>::InitBeforeRun()
{
    EvolutionaryAlgorithm<T>::InitBeforeRun();
    //std::shuffle(m_index_vec.begin(), m_index_vec.end(), EA::m_random_engine);
}

template <class T>
void DifferentialEvolution<T>::Breed()
{
    // mutate each? solution in population, get the transition solution
    int sz = DifferentialEvolution<T>::m_population.size();
    EA::m_offspring.resize(sz);
    std::uniform_int_distribution<int> random_dis(0, sz - 1);
    for (size_t i = 0; i < sz; ++i)
    {
        //? Here I can not ensure the 3 random numbers are not the same. Should I ensure it?
        int index_a = random_dis(EA::m_random_engine);
        int index_b = random_dis(EA::m_random_engine);
        Solution<T> &material_a = EA::m_population[index_a];
        Solution<T> &material_b = EA::m_population[index_b];
        Solution<T> child = Mutate(EA::m_population[i], material_a, material_b);
        EvolutionaryAlgorithm<T>::m_offspring[i] = Crossover(EA::m_population[i], child);
    }
}

} // namespace sc2

#endif // DIFFERENTIAL_EVOLUTION