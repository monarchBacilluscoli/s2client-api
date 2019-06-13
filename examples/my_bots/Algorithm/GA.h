//

#ifndef GA_H
#define GA_H

#include <vector>
#include <functional>
#include <cassert>
#include <sc2api/sc2_common.h> //! I want to use the random functions. They are easy to use and I love them!
#include <numeric>

template<class T>
struct Solution
{
    std::vector<float> objectives;
    std::vector<T> variable;
    int rank = 0;

    bool operator==(const Solution& rhs) const {
        return variable == rhs.variable;
    }
    bool operator!=(const Solution& rhs) const {
        return !(*this == rhs);
    }

    Solution<T>() = default;
    Solution<T>(int variable_size, int objective_size) {
        variable.resize(variable_size);
        objectives.resize(objective_size);
    }
    Solution<T>(int variable_size) {
        variable.resize(variable_size);
        objectives.resize(1);
    }

    //! two compares for the use of sort(), descending order 
    // return true means the orders of the two items keep unchanged
    static bool multi_greater(const Solution<T>& a, const Solution<T>& b);
    static bool sum_greater(const Solution<T>& a, const Solution<T>& b);
};

template<class T>
class GA {

    using Population = std::vector<Solution<T>>;
    using Evaluator = std::function<float(const std::vector<T>&)>;
    using Compare = std::function<bool(const Solution<T>&, const Solution<T>&)>;

public:
    // after you calling the default constructor, I hope you can call the Initialize()
    GA() = default;
    /*GA(std::vector<Evaluator>& evaluators, float crossover_rate = 1.f, float mutate_rate = 0.2f, int population_size = 20, Compare compare = Solution<T>::sum_greater, int max_generation = 100, float reproduce_rate = 1.f) :m_evaluators(evaluators), m_cross_over_rate(crossover_rate), m_mutate_rate(mutate_rate), m_population_size(population_size), m_compare(compare), m_max_generation(max_generation), m_reproduce_rate(reproduce_rate) {
        assert(population_size % 2 == 0);
        m_population.resize(population_size);
    };*/
    ~GA() = default; //? whether or not the reference of evaluators can effect the destruction.

    void SetMaxGeneration(int max_generation) {
        m_max_generation = max_generation;
    }

    void SetCrossOverRate(int crossover_rate) {
        m_cross_over_rate = crossover_rate;
    }

    void SetMutateRate(int mutate_rate) {
        m_mutate_rate = mutate_rate;
    }

    virtual void SetPopulationSize(int population_size) {
        m_population_size = population_size;
        m_population.resize(m_population_size);
    }

    void SetReproduceRate(int reproduce_rate) {
        m_reproduce_rate = reproduce_rate;
    }

    void SetCompare(Compare compare) {
        m_compare = compare;
    }

    void SetEvaluator(Evaluator* evaluator) {
        m_evaluators = evaluator;
    }

    //! run the algorithm and return the final population to choose
    virtual Population Run();

protected:
    //! simply calls the GenerateSolution() repeatedly to generate original population
    virtual void GenerateSolutions(Population& pop, int size);
    //! two parents generate a unmutated soluton
    virtual std::vector<Solution<T>> CrossOver(const Solution<T>& a, const Solution<T>& b);
    //virtual Solution<T> Mutate(const Solution<T>& s); //? mutate must be implemented by users
    //? pure virtual
    virtual void Mutate(Solution<T>& s) = 0;
    //! a population generate another population
    // The spring_size need to be smaller than the size of parents
    virtual void Reproduce(const Population& parents, Population& offspring, int spring_size);
    //! Just calls the EvaluateSingleSolution() repeatedly to evaluate a population
    virtual void Evaluate(Population& p);
    //! According to the compare to sort the population, no multi-objective rank being considered
    virtual void SortSolutions(Population& p, const Compare& compare);

    //! two parents generate two children by crossover and mutation
    virtual std::vector<Solution<T>> Produce(const Solution<T>& a, const Solution<T>& b);
    //! Calls those evaluators to evaluate one solution
    virtual void EvaluateSingleSolution(Solution<T>& solution);
    //! Generate one solution
    //? pure virtual
    virtual Solution<T> GenerateSolution() = 0;

    //! Settings
    int m_max_generation = 20;
    //! Controls how many parents ...
    //? this setting is OK, but the process is not right, since I shouldn't let the parents to generate a pair of just the same children, instead, I should pass them to another pair of parents
    //todo Here is a todo of modification for 
    float m_cross_over_rate = 1.f;
    //! Controls how many offspring solutions should be mutated
    float m_mutate_rate = 0.2f;
    //! Controls the population size after each selection
    int m_population_size = 50;
    //! Controls the ratio between the parents population and the offspring population in each reproduction
    float m_reproduce_rate = 1.f;

    Compare m_compare = Solution<T>::sum_greater;
    Evaluator default_evaluator = [](const std::vector<T> variables)->float {return 0; };
    std::vector<Evaluator*> eva_v = { &default_evaluator };
    std::vector<Evaluator*> m_evaluators = eva_v; // for easy to use

    //! Runtime data
    Population m_population;
};

template<class T>
inline std::vector<Solution<T>> GA<T>::Run()
{
    m_population.resize(m_population_size);
    GenerateSolutions(m_population, m_population_size);
    Evaluate(m_population);
    SortSolutions(m_population, m_compare);
    for (size_t i = 0; i < m_max_generation; i++)
    {
        Population offspring;
        Reproduce(m_population, offspring, m_reproduce_rate * m_population_size);
        Evaluate(offspring);
        m_population.insert(m_population.begin(), offspring.begin(), offspring.end());
        SortSolutions(m_population, m_compare);
        m_population.erase(m_population.begin() + m_population_size, m_population.end()); //
    }
    return m_population;

}

template<class T>
inline void GA<T>::GenerateSolutions(Population& pop, int size)
{
    for (size_t i = 0; i < size; i++) {
        pop[i] = GenerateSolution();
    }
}

template<class T>
inline std::vector<Solution<T>> GA<T>::CrossOver(const Solution<T>& a, const Solution<T>& b)
{
    //! you can use #define NDBUG to disabled assert()
    assert(a.variable.size() > 0 && a.variable.size() == b.variable.size());
    std::vector<Solution<T>> offspring = { a,b };
    size_t start = sc2::GetRandomInteger(0, a.variable.size() - 1);
    size_t end = sc2::GetRandomInteger(0, a.variable.size() - 1);
    if (start > end) {
        std::swap(start, end);
    }
    for (size_t i = start; i < end; i++)
    {
        std::swap(offspring[0].variable[i], offspring[1].variable[i]);
    }
    return offspring;
}

//template<class T>
//inline void GA<T>::Mutate(Solution<T>& s)
//{
//    //! default was just used when T == float, only for test
//    assert(typeid(T) == typeid(float));
//    sc2::GetRandomEntry(s.variable) += 0.1f;
//}

template<class T>
inline std::vector<Solution<T>> GA<T>::Produce(const Solution<T>& a, const Solution<T>& b)
{
    std::vector<Solution<T>> children;
    if (sc2::GetRandomFraction() < m_cross_over_rate) { //? it should't be here, instead, it should be outside
        children = CrossOver(a, b);
    }
    else {
        children = { a,b };
    }
    for (Solution<T>& c: children)
    {
        if (sc2::GetRandomFraction() < m_mutate_rate) {
            Mutate(c);
        }
    }
    return children;
}

template<class T>
inline void GA<T>::Reproduce(const Population& parents, Population& offspring, int spring_size)
{
    assert(spring_size <= parents.size()); //! for now, I can not reproduce a larger offspring population
    offspring.resize(spring_size);
    for (size_t i = 0; i < spring_size; i+=2)
    {
        std::vector<Solution<T>> instant_children = Produce(parents[i], parents[i + 1]);
        offspring[i] = instant_children[0];
        if (i + 1 < spring_size) {
            offspring[i + 1] = instant_children[1];
        }
    }
}

template<class T>
inline void GA<T>::Evaluate(Population& p)
{
    for (Solution<T>& s: p)
    {
        EvaluateSingleSolution(s);
    }
}

template<class T>
inline void GA<T>::SortSolutions(Population& p, const Compare& compare)
{
    std::sort(p.begin(), p.end(), compare);
}


template<class T>
inline void GA<T>::EvaluateSingleSolution(Solution<T>& solution)
{
    //assert(solution.objectives.size() == m_evaluators.size()); //? or I can add, but this will effect the performance, so you'd better set the settings properly before the run
    //for (size_t i = 0; i < m_evaluators.size(); i++)
    //{
    //    solution.objectives[i] = m_evaluators[i](solution.variable);
    //}
}

template<class T>
inline bool Solution<T>::multi_greater(const Solution<T>& a, const Solution<T>& b)
{
    bool equal = true;
    // If any of the b's dimensions is bigger than a, return false
    for (size_t i = 0; i < a.objectives.size(); i++) {
        if (a.objectives[i] < b.objectives[i]) {
            return false;
        }
        else if (equal && fabsf(a.objectives[i] - b.objectives[i]) > 0.000001) {
            equal = false;
        }
    }
    // If all the demensions are equal, return false
    if (equal) {
        return false;
    }
    // else return true
    return true;
}

template<class T>
inline bool Solution<T>::sum_greater(const Solution<T>& a, const Solution<T>& b)
{
    return std::accumulate(a.objectives.begin(), a.objectives.end(), 0.f) > std::accumulate(b.objectives.begin(), b.objectives.end(), 0.f);
}

#endif // !GA_H
