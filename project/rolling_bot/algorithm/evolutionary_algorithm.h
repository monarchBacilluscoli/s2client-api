#ifndef EVOLUTIONARY_ALGORITHM_H
#define EVOLUTIONARY_ALGORITHM_H

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

protected:
    //! some settings
    int m_objective_size = 1;
    int m_max_generation = 10;
    int m_population_size = 50;
    int m_current_generation = 0;
    std::vector<std::string> m_objective_names{std::vector<std::string>(m_objective_size)};

    //! data
    Population m_population{};
    Population m_offspring{};

    //! methods
    std::list<std::vector<std::vector<float>>> m_history_objs{}; // for debug or record use
    std::vector<std::vector<float>> m_history_objs_ave{};        // one list one objective
    std::vector<std::vector<float>> m_history_objs_best{};       // one list one objective
    std::vector<std::vector<float>> m_history_objs_worst{};      // one list one objective
    LineChartRenderer2D m_overall_evolution_status_renderer;
    std::mt19937 m_random_engine{0};

public:
    EvolutionaryAlgorithm() = default;
    //todo a constructor with all parameters
    EvolutionaryAlgorithm(int objective_size, int max_generation, int population_size, int random_seed = 0, std::vector<std::string> objective_names = std::vector<std::string>()) : m_max_generation{max_generation}, m_population_size{population_size}, m_objective_size{objective_size}, m_random_engine{random_seed}
    {
        m_overall_evolution_status_renderer.SetTitle("Evolution Status");
    };
    virtual ~EvolutionaryAlgorithm() = default;

    void SetMaxGeneration(int max_ge);
    void SetPopulationSize(int pop_size);
    void SetObjectiveSize(int obj_size);
    void SetRandomEngineSeed(int seed);
    void SetObjectiveNames(const std::vector<std::string> &objective_names);

    int GetMaxGeneration() const { return m_max_generation; };
    int GetPopulationSize() const { return m_population_size; };
    int GetObjectiveSize() const { return m_objective_size; };
    const Population &GetPopulation() const { return m_population; };
    int GetCurrentGeneration() const { return m_current_generation; };

    virtual Population &Run();

protected:
    virtual void InitBeforeRun();

    virtual void Generate() = 0; // Generate the initial population
    virtual void Breed() = 0;    // use parent population to generate child population
    virtual void Evaluate() = 0; // Evaluate all solutions
    virtual void Select() = 0;   // select which solutions to enter into the next generation
    //todo maybe I need a ActionAfterEachGeneration() to store all the objs or somthing else
    virtual void ActionAfterEachGeneration();
    virtual void RecordRunningData();
    virtual void ShowGraphEachGeneration(); //todo show the overall status

    void RecordObjectives();
    virtual void ShowOverallStatusGraphEachGeneration();
    virtual void ShowSolutionDistribution(int showed_generations_count) = 0;
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
void EvolutionaryAlgorithm<T>::SetObjectiveNames(const std::vector<std::string> &objective_names)
{
    m_objective_names = objective_names;
}

template <class T>
void EvolutionaryAlgorithm<T>::InitBeforeRun()
{
    m_population.clear();
    m_population.resize(m_population_size);
    m_overall_evolution_status_renderer.SetXRange(0, m_max_generation);
}

template <class T>
std::vector<Solution<T>> &EvolutionaryAlgorithm<T>::Run()
{
    InitBeforeRun();
    Generate();
    Evaluate();
    for (m_current_generation = 1; m_current_generation <= m_max_generation; ++m_current_generation)
    {
        Breed();
        Evaluate();
        Select();
        ActionAfterEachGeneration();
    }
    return m_population;
}

template <class T>
void EvolutionaryAlgorithm<T>::ActionAfterEachGeneration()
{
    RecordRunningData();
    ShowGraphEachGeneration();
}

template <class T>
void EvolutionaryAlgorithm<T>::RecordRunningData()
{
    RecordObjectives();
}

template <class T>
void EvolutionaryAlgorithm<T>::RecordObjectives()
{
    // all the objs
    int pop_sz = m_population.size();
    m_history_objs.emplace_back(std::vector<std::vector<float>>(pop_sz, std::vector<float>(m_objective_size)));
    std::vector<std::vector<float>> &current_generation_objs = m_history_objs.back();
    for (size_t i = 0; i < pop_sz; ++i)
    {
        current_generation_objs[i] = m_population[i].objectives;
    }
    // objs statistical data
    m_history_objs_ave.emplace_back(std::vector<float>(m_objective_size));
    m_history_objs_best.emplace_back(std::vector<float>(m_objective_size));
    m_history_objs_worst.emplace_back(std::vector<float>(m_objective_size));
    std::vector<float> &current_ave_objs = m_history_objs_ave.back();
    std::vector<float> &current_best_objs = m_history_objs_best.back();
    std::vector<float> &current_worst_objs = m_history_objs_worst.back();
    for (size_t i = 0; i < m_objective_size; ++i)
    {
        // ave
        current_ave_objs[i] = std::accumulate(current_generation_objs.begin(), current_generation_objs.end(), 0.f, [i](float initial_value, const std::vector<float> &so2) -> float {
            return initial_value + so2[i];
        });
        current_ave_objs[i] /= pop_sz;
        // best
        auto best_iter_i = std::max_element(current_generation_objs.begin(), current_generation_objs.end(), [i](const std::vector<float> &so1, const std::vector<float> so2) -> bool {
            return so1[i] < so2[i];
        });
        current_best_objs[i] = (*best_iter_i)[i];
        // worst
        auto worst_iter_i = std::min_element(current_generation_objs.begin(), current_generation_objs.end(), [i](const std::vector<float> &so1, const std::vector<float> &so2) -> bool {
            return so1[i] > so2[i];
        });
        current_worst_objs[i] = (*worst_iter_i)[i];
    }
}

template <class T>
void EvolutionaryAlgorithm<T>::ShowGraphEachGeneration()
{
    //todo show the evolution status of the algorithm
    ShowOverallStatusGraphEachGeneration();
    ShowSolutionDistribution(3); // show the last generations objs distribution
}

template <class T>
void EvolutionaryAlgorithm<T>::ShowOverallStatusGraphEachGeneration()
{
    std::vector<std::vector<float>> data;
    data.reserve(2 * 3 * m_objective_size);                                            // I don't know if it is useful
    data.insert(data.end(), m_history_objs_ave.begin(), m_history_objs_ave.end());     // insert objective_size's vectors in data
    data.insert(data.end(), m_history_objs_best.begin(), m_history_objs_best.end());   // insert objective_size's vectors in data
    data.insert(data.end(), m_history_objs_worst.begin(), m_history_objs_worst.end()); // insert objective_size's vectors in data

    std::vector<float> generation_indices(m_current_generation + 1);
    std::iota(generation_indices.begin(), generation_indices.end(), 0);
    std::vector<std::string> line_names(m_objective_size * 3);
    // names order is obj1_ave, obj2_ave,.. objN_ave, obj1_best, ..., obj1_worst ,objN_worst
    for (size_t i = 0; i < m_objective_size; ++i)
    {
        line_names[0 + i] = m_objective_names[i] + " average";
        line_names[m_objective_size + i] = m_objective_names[i] + " best";
        line_names[m_objective_size * 2 + i] = m_objective_names[i] + " worst";
    }
    m_overall_evolution_status_renderer.Show(data, generation_indices, line_names);
}

} // namespace sc2

#endif //EVOLUTIONARY_ALGORITHM_H