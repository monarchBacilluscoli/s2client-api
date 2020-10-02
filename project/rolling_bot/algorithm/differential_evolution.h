#ifndef DIFFERENTIAL_EVOLUTION_H
#define DIFFERENTIAL_EVOLUTION_H

#include <algorithm>
#include "evolutionary_algorithm.h"
#ifdef DEBUG
#include "../simulator/command.h"
#endif

namespace sc2
{
    template <class T, template <typename> class TSolution>
    class DifferentialEvolution : virtual public EvolutionaryAlgorithm<T, TSolution>
    {
    public:
        using EA = EvolutionaryAlgorithm<T, TSolution>;

    protected:
        // settings
        float m_scale_factor = .5f;
        float m_crossover_rate = .5f;

    public:
        DifferentialEvolution() : EA(){};
        //todo constructor with all parameters
        DifferentialEvolution(int objective_size,
                              int max_generation,
                              int population_size,
                              float scale_factor = .5,
                              float crossover_rate = .5f, int random_seed = 0, std::vector<std::string> objective_names = std::vector<std::string>()) : EA(objective_size, max_generation, population_size, random_seed, objective_names), m_scale_factor(scale_factor), m_crossover_rate(crossover_rate){};
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
        virtual void Breed() override; // Get all populations bred

        virtual TSolution<T> Mutate(const TSolution<T> &base_sol, const TSolution<T> &material_sol1, const TSolution<T> &material_sol2) = 0;
        virtual void Crossover(const TSolution<T> &parent, TSolution<T> &child) = 0; // can not be implemented, since there are so many solution types
        virtual void Breed_(int pop_index = 0);                                      // Get a indexed population bred
    };

    template <class T, template <typename> class TSolution>
    void DifferentialEvolution<T, TSolution>::InitBeforeRun()
    {
        EA::InitBeforeRun();
        //todo the code only belonging to DE<T>
    }

    template <class T, template <typename> class TSolution>
    void DifferentialEvolution<T, TSolution>::InitOnlySelfMembersBeforeRun() {}

    template <class T, template <typename> class TSolution>
    void DifferentialEvolution<T, TSolution>::Breed_(int pop_index)
    {
        // mutate each? solution in population, get the transition solution
        typename EA::Population &current_pop = EA::m_populations[pop_index];
        typename EA::Population &current_off = EA::m_offsprings[pop_index];
        int sz = current_pop.size();
        if (EA::m_offsprings.size() < pop_index)
        {
            EA::m_offsprings.resize(pop_index, Population());
        }
        current_off.resize(EA::m_population_size - EA::m_elite_size, TSolution<T>(0, EA::m_objective_size)); //? 有疑问
        std::uniform_int_distribution<int> random_dis(0, sz - 1);
        for (size_t j = 0; j < EA::m_population_size - EA::m_elite_size; ++j)
        {
            //? Here I can not ensure the 3 random numbers are not the same. Should I ensure it?
            int index_a = random_dis(EA::m_random_engine);
            int index_b = random_dis(EA::m_random_engine);
            int index_src = j % EA::m_elite_size;
            current_off[j] = Mutate(current_pop[index_src], current_pop[index_a], current_pop[index_b]);
            Crossover(current_pop[index_src], current_off[j]);
        }
        return;
    }

    template <class T, template <typename> class TSolution>
    void DifferentialEvolution<T, TSolution>::Breed()
    {
        //todo 这里breed是要breed满population size
        for (size_t i = 0; i < EA::m_populations.size(); ++i)
        {
            Breed_(i);
        }
        for (int i = 0; i < EA::m_populations.size(); ++i)
        {
            auto &current_pop = EA::m_populations[i];
            auto &current_off = EA::m_offsprings[i];
            current_pop.insert(current_pop.begin() + EA::m_elite_size, current_off.begin(), current_off.end());
            current_pop.resize(EA::m_population_size);
        }
    }

} // namespace sc2

#endif // DIFFERENTIAL_EVOLUTION_H
