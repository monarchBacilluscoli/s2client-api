//! This is the base class of core algorithm rolling GA
#ifndef GA_H
#define GA_H

#include<vector>
#include<numeric>

template<class T>
struct Solution
{
    std::vector<float> objectives;
    std::vector<T> variable;
    int rank = 0; // I haven't used it yet

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
    GA() = default;

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

template <class T>
bool Solution<T>::multi_greater(const Solution<T> &a, const Solution<T> &b)
{
    bool equal = true;
    // If any of the b's dimensions is bigger than a, return false
    for (size_t i = 0; i < a.objectives.size(); i++)
    {
        if (a.objectives[i] < b.objectives[i])
        {
            return false;
        }
        else if (equal && fabsf(a.objectives[i] - b.objectives[i]) > 0.000001)
        {
            equal = false;
        }
    }
    // If all the demensions are equal, return false
    if (equal)
    {
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

#endif //GA_H