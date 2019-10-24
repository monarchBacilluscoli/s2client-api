#ifndef EVOLUTIONARY_ALGORITHM
#define EVOLUTIONARY_ALGORITHM

#include <vector>
#include <list>
#include <random>
#include "solution.h"
#include "../methods/graph_renderer.h"

namespace sc2
{

template <class T>
class EvolutionaryAlgorithm
{
public:
    using Population = std::vector<Solution<T>>;

public:
    //! some settings
    int m_max_generation = 10;
    int m_population_size = 50;
    int m_current_generation = 0;
    int m_objective_size = 1;

    //! data
    Population m_population{};
    Population m_offspring{};

    //! methods
    std::list<std::vector<std::vector<float>>> m_history_objs{}; // for debug or record use
    std::mt19937 m_random_engine{0};

public:
    EvolutionaryAlgorithm() = default;
    //todo a constructor with all parameters
    EvolutionaryAlgorithm(int max_generation, int population_size, int m_objecitve_size, int random_seed = 0) : m_max_generation{max_generation}, m_population_size{population_size}, m_objective_size{m_objective_size}, m_random_engine{random_seed} {};
    virtual ~EvolutionaryAlgorithm() = default;

    void SetMaxGeneration(int max_ge);
    void SetPopulationSize(int pop_size);
    void SetObjectiveSize(int obj_size);
    void SetRandomEngineSeed(int seed);

    int GetMaxGeneration() { return m_max_generation; };
    int GetPopulationSize() { return m_population_size; };
    int GetObjectiveSize() { return m_objective_size; };
    const Population &GetPopulation() { return m_population; };
    int GetCurrentGeneration() { return m_current_generation; };

    virtual void Run();

protected:
    virtual void InitBeforeRun();

    virtual void Generate() = 0; // Generate the initial population
    virtual void Breed() = 0; // use parent population to generate child population
    virtual void Evaluate() = 0; // Evaluate all solutions
    virtual void Sort() = 0; // sort all solutions, prepare them for select
    virtual void Select() = 0; // select which solutions to enter into the next generation
    //todo maybe I need a ActionAfterEachGeneration() to store all the objs or somthing else
    virtual void ShowGraphEachGeneration(){};
};

template <class T>
void EvolutionaryAlgorithm<T>::SetMaxGeneration(int max_ge)
{
    m_population_size = max_ge;
}

template <class T>
void EvolutionaryAlgorithm<T>::SetPopulationSize(int pop_size)
{
    m_population_size = pop_size;
}

template <class T>
void EvolutionaryAlgorithm<T>::SetObjectiveSize(int obj_size)
{
    m_objective_size = obj_size;
}

template <class T>
void EvolutionaryAlgorithm<T>::SetRandomEngineSeed(int seed)
{
    m_random_engine.seed(seed);
}

template <class T>
void EvolutionaryAlgorithm<T>::InitBeforeRun()
{
    m_population.clear();
    m_population.resize(m_population_size);
}

template <class T>
void EvolutionaryAlgorithm<T>::Run()
{
    InitBeforeRun();
    Generate();
    Evaluate();
    Sort();
    for (m_current_generation = 1; m_current_generation < m_max_generation; ++m_current_generation)
    {
        Breed();
        Evaluate();
        Sort();
        ShowGraphEachGeneration();
    }
}
} // namespace sc2

#endif //EVOLUTIONARY_ALGORITHM
