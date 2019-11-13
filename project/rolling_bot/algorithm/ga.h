//! This is the base class of core algorithm rolling GA
#ifndef GA_H
#define GA_H

#include <sc2api/sc2_common.h>
#include <cassert>
#include <vector>
#include <functional>
#include "solution.h"
#include "algorithm"

template <class T>
class GA
{
    using Population = std::vector<Solution<T>>;
    using Evaluator = std::function<float(const std::vector<T> &)>;
    using Compare = std::function<bool(const Solution<T> &, const Solution<T> &)>;

public:
    GA() = default;

    GA(size_t population_size) : m_population_size(population_size){};

    ~GA() = default; //? whether or not the reference of evaluators can effect the destruction.

    void SetMaxGeneration(int max_generation)
    {
        m_max_generation = max_generation;
    }

    void SetCrossOverRate(int crossover_rate)
    {
        m_cross_over_rate = crossover_rate;
    }

    void SetMutateRate(int mutate_rate)
    {
        m_mutate_rate = mutate_rate;
    }

    virtual void SetPopulationSize(int population_size)
    {
        m_population_size = population_size;
        m_population.resize(m_population_size);
    }

    void SetReproduceRate(int reproduce_rate)
    {
        m_reproduce_rate = reproduce_rate;
    }

    void SetCompare(Compare compare)
    {
        m_compare = compare;
    }

    void SetEvaluator(Evaluator *evaluator)
    {
        m_evaluators.resize(1);
        m_evaluators[0] = evaluator;
    }

    void SetEvaluators(std::vector<Evaluator *> evaluators)
    {
        m_evaluators = evaluators;
    }

    //! run the algorithm and return the final population to choose
    virtual Population Run();

protected:
    //! simply calls the GenerateSolution() repeatedly to generate original population
    virtual void Generate(Population &pop, int size);
    //! two parents generate a unmutated soluton
    virtual std::vector<Solution<T>> CrossOver(const Solution<T> &a, const Solution<T> &b);
    //! pure virtual mutate a solution
    virtual void Mutate(Solution<T> &s) = 0;
    //! a population generate another population
    // The spring_size need to be smaller than the size of parents
    virtual void Reproduce(const Population &parents, Population &offspring, int spring_size);
    //! Just calls the EvaluateSingleSolution() repeatedly to evaluate a population
    virtual void Evaluate(Population &p);
    //! According to the compare to sort the population, no multi-objective rank being considered
    // virtual void SortSolutions(Population &p, const Compare &compare);
    virtual void SortSolutions(Population &p) = 0;

    //! two parents generate two children by crossover and mutation
    virtual std::vector<Solution<T>> Produce(const Solution<T> &a, const Solution<T> &b);
    //! Calls those evaluators to evaluate one solution
    virtual void EvaluateSingleSolution(Solution<T> &solution);
    //! Generate one solution
    //? pure virtual
    virtual Solution<T> GenerateSolution() = 0;

    //! Print the graph, pure virtual method
    virtual void ShowGraphEachGeneration(){};
    //!
    virtual void InitBeforeRun()
    {
        m_population.clear();
        m_population.resize(m_population_size);
    };

    //! Settings
    int m_max_generation = 30;
    //! Controls how many parents ...
    float m_cross_over_rate = 1.f;
    //! Controls how many offspring solutions should be mutated
    float m_mutate_rate = 0.5f;
    //! Controls the population size after each selection
    int m_population_size = 50;
    //! Controls the ratio between the parents population and the offspring population in each reproduction
    float m_reproduce_rate = 1.f;

    Compare m_compare = Solution<T>::sum_greater;
    std::vector<Evaluator *> m_evaluators = {nullptr}; // for easy to use

    //! Runtime data
    Population m_population;
    //! generation count
    size_t m_current_generation = 0;
};

template <class T>
std::vector<Solution<T>> GA<T>::Run()
{
    InitBeforeRun();
    Generate(m_population, m_population_size);
    Evaluate(m_population);
    SortSolutions(m_population);
    for (m_current_generation = 1; m_current_generation <= m_max_generation; m_current_generation++)
    {
        //todo According to current generation, I can adjust the mutation rate
        Population offspring;
        Reproduce(m_population, offspring, m_reproduce_rate * m_population_size);
        Evaluate(offspring);
        m_population.insert(m_population.begin(), offspring.begin(), offspring.end());
        SortSolutions(m_population);
        m_population.erase(m_population.begin() + m_population_size, m_population.end()); //
        ShowGraphEachGeneration();
    }
    return m_population;
}

template <class T>
inline void GA<T>::Generate(Population &pop, int size)
{
    for (size_t i = 0; i < size; i++)
    {
        pop[i] = GenerateSolution();
    }
}

template <class T>
inline std::vector<Solution<T>> GA<T>::CrossOver(const Solution<T> &a, const Solution<T> &b)
{
    //! you can use #define NDBUG to disabled assert()
    assert(a.variable.size() > 0 && a.variable.size() == b.variable.size());
    std::vector<Solution<T>> offspring = {a, b};
    size_t start = sc2::GetRandomInteger(0, a.variable.size() - 1);
    size_t end = sc2::GetRandomInteger(0, a.variable.size() - 1);
    if (start > end)
    {
        std::swap(start, end);
    }
    for (size_t i = start; i < end; i++)
    {
        std::swap(offspring[0].variable[i], offspring[1].variable[i]);
    }
    return offspring;
}

template <class T>
inline std::vector<Solution<T>> GA<T>::Produce(const Solution<T> &a, const Solution<T> &b)
{
    std::vector<Solution<T>> children;
    if (sc2::GetRandomFraction() < m_cross_over_rate)
    { //? it should't be here, instead, it should be outside
        children = CrossOver(a, b);
    }
    else
    {
        children = {a, b};
    }
    for (Solution<T> &c : children)
    {
        if (sc2::GetRandomFraction() < m_mutate_rate)
        {
            Mutate(c);
        }
    }
    return children;
}

template <class T>
inline void GA<T>::Reproduce(const Population &parents, Population &offspring, int spring_size)
{
    assert(spring_size <= parents.size()); //! for now, I can not reproduce a larger offspring population
    offspring.resize(spring_size);
    for (size_t i = 0; i < spring_size; i += 2)
    {
        std::vector<Solution<T>> instant_children = Produce(parents[i], parents[i + 1]);
        offspring[i] = instant_children[0];
        if (i + 1 < spring_size)
        {
            offspring[i + 1] = instant_children[1];
        }
    }
}

template <class T>
inline void GA<T>::Evaluate(Population &p)
{
    for (Solution<T> &s : p)
    {
        EvaluateSingleSolution(s);
    }
}

template <class T>
inline void GA<T>::EvaluateSingleSolution(Solution<T> &solution)
{
    assert(solution.objectives.size() == m_evaluators.size()); //? or I can add, but this will effect the performance, so you'd better set the settings properly before the run
    for (size_t i = 0; i < m_evaluators.size(); i++)
    {
        solution.objectives[i] = (*m_evaluators[i])(solution.variable);
    }
}

#endif //GA_HSelectNearestUnitFromPoint