#ifndef EVOLUTIONARY_ALGORITHM_H
#define EVOLUTIONARY_ALGORITHM_H

#include "../../global_defines.h"

#include <vector>
#include <list>
#include <map>
#include <functional>
#include <random>
#include <iostream>
#include <memory>
#include <fstream>
#include <string>

#include "solution.h"
#ifdef USE_GRAPHICS
#include "../methods/graph_renderer.h"
#endif // USE_GRAPHICS
#ifdef DEBUG
#include "../simulator/command.h"
#endif // DEBUG
#include "terminator.h"
#include "../../debug_use.h"

namespace sc2
{
    enum class TERMINATION_CONDITION
    {
        MAX_GENERATION = 0,
        CONVERGENCE = 1,
        MAX_EVALUATION = 2,
    };
    template <class T, template <typename> class TSolution = Solution> // T is the variable type and TSolution is the solution type
    class EvolutionaryAlgorithm
    {

    public:
        using Population = std::vector<TSolution<T>>;

    protected:
        using TB = TerminatorBase<T, TSolution>;
        using CT = ConvergenceTerminator<T, TSolution>;
        using MGT = MaxGenerationTerminator<T, TSolution>;
        using MET = MaxEvaluationTerminator<T, TSolution>;
        //! /etc
        bool m_is_output_file = false;
        int m_objective_size = 1;
        int m_is_enemy_pop_evo = false;
        int m_population_size = 50;
        int m_elite_size = m_population_size * 0.5f;
        int m_sub_pop_size = MAX_SIM_SIZE / m_population_size > 1 ? MAX_SIM_SIZE / m_population_size : 1;
        //! /var
        int m_current_generation = 0;
        int m_current_evaluation = 0;
        int m_max_evaluation = 0;
        //! data
        std::vector<Population> m_populations{1, Population()};
        std::vector<Population> m_offsprings{1, Population()};

        TERMINATION_CONDITION m_termination_condition = TERMINATION_CONDITION::CONVERGENCE;
        std::vector<std::string> m_objective_names{std::vector<std::string>(m_objective_size)};
        std::map<TERMINATION_CONDITION, std::shared_ptr<TB>> m_termination_conditions = {
            // if true, stop loop
            {TERMINATION_CONDITION::MAX_GENERATION, std::make_shared<MGT>(*this)},
            {TERMINATION_CONDITION::CONVERGENCE, std::make_shared<CT>(*this)}, /* add it by yourself */
            {TERMINATION_CONDITION::MAX_EVALUATION, std::make_shared<MET>(*this)},
        };

        //! methods
        std::string m_output_file_path = CurrentFolder() + "/obj_record.txt";
        std::list<std::vector<std::vector<float>>> m_history_objs{}; // for debug or record use, list:generation, outter vec:individual, inner vec:objs
        std::vector<std::vector<float>> m_history_objs_ave{};        // outter vec: objs, inner vec: generation
        std::vector<std::vector<float>> m_history_objs_best{};       // outter vec: objs, inner vec: generation
        std::vector<std::vector<float>> m_history_objs_worst{};      // outter vec: objs, inner vec: generation
#ifdef USE_GRAPHICS
        LineChartRenderer2D m_overall_evolution_status_renderer;
#endif //USE_GRAPHICS
        std::mt19937 m_random_engine{0};

    public:
        EvolutionaryAlgorithm() = default;
        // a constructor with all parameters
        EvolutionaryAlgorithm(int objective_size,
                              int max_generation,
                              int population_size,
                              int random_seed = 0,
                              std::vector<std::string> objective_names = std::vector<std::string>(),
                              int num_pop = 2) : m_termination_conditions{
                                                     // if true, stop loop
                                                     {TERMINATION_CONDITION::MAX_GENERATION, std::make_shared<MGT>(*this, max_generation)},
                                                     {TERMINATION_CONDITION::CONVERGENCE, std::make_shared<CT>(*this)}, /* add it by yourself */
                                                     {TERMINATION_CONDITION::MAX_EVALUATION, std::make_shared<MET>(*this)},
                                                 },
                                                 m_population_size(population_size), m_objective_size(objective_size), m_random_engine{random_seed}, m_history_objs_ave(objective_size), m_history_objs_best(objective_size), m_history_objs_worst(objective_size), m_objective_names(objective_size), m_populations(num_pop, Population(m_population_size)), m_offsprings(num_pop, Population()), m_elite_size(m_population_size * 0.5f)
        {
#ifdef USE_GRAPHICS
            m_overall_evolution_status_renderer.SetTitle("Evolution Status");
#endif //USE_GRAPHICS
        };
        virtual ~EvolutionaryAlgorithm() = default;

        void SetPopulationSize(int pop_size) { m_population_size = pop_size; };
        void SetObjectiveSize(int obj_size);
        void SetObjectiveNames(const std::vector<std::string> &objective_names);
        void SetRandomEngineSeed(int seed) { m_random_engine.seed(seed); };
        void SetTerminationCondition(TERMINATION_CONDITION termination_condition) { m_termination_condition = termination_condition; };
        void SetMaxGeneration(int max_generation) { std::dynamic_pointer_cast<MGT>(m_termination_conditions[TERMINATION_CONDITION::MAX_GENERATION])->SetMaxGeneration(max_generation); };
        void SetUseOutputFile(bool use) { m_is_output_file = use; };
        void SetOutputPath(const std::string &path) { m_output_file_path = path; };

        int GetPopulationSize() const { return m_population_size; };
        int GetObjectiveSize() const { return m_objective_size; };
        const Population &GetPopulation() const { return m_populations[0]; };
        TERMINATION_CONDITION GetTerminationCondition() const { return m_termination_condition; };
        int GetCurrentGeneration() const { return m_current_generation; };
        virtual int CurrentEvaluation() const { return 0; };
        int GetMaxGeneration() { return std::dynamic_pointer_cast<MGT>(m_termination_conditions[TERMINATION_CONDITION::MAX_GENERATION])->MaxGeneration(); };
        std::vector<float> GetLastObjsAverage() const;
        std::vector<float> GetLastObjsBest() const;
        std::vector<float> GetLastObjsWorst() const;
        virtual std::shared_ptr<TB> TerminationCondition(TERMINATION_CONDITION termination_condition);
        virtual std::shared_ptr<CT> ConvergenceTermination() { return std::dynamic_pointer_cast<CT>(m_termination_conditions[TERMINATION_CONDITION::CONVERGENCE]); };
        bool GetUseOutput() { return m_is_output_file; };
        std::string GetOutputPath() { return m_output_file_path; };

        virtual Population Run();

    protected:
        virtual void InitBeforeRun();

        virtual void Generate() = 0; // Generate the initial population
        virtual void Breed() = 0;    // use parent population to generate child population
        virtual void Evaluate() = 0; // Evaluate all solutions
        virtual void Select() = 0;   // select which solutions to enter into the next generation
        virtual void ActionAfterEachGeneration();
        virtual void RecordRunningData();
        virtual void RecordObjectives();
        virtual void RecordObjectives_(int pop_index);

        virtual void OutputToConsoleEachGeneration(std::ostream &out);
        virtual void OutputCurrentObjectives(std::ostream &out);
        virtual void OutputAllHistoryObjectives(std::ostream &out);
#ifdef USE_GRAPHICS
        virtual void ShowGraphEachGeneration();
        virtual void ShowOverallStatusGraphEachGeneration();
        virtual void ShowSolutionDistribution(int showed_generations_count) = 0;
#endif //USE_GRAPHICS

        bool CheckIfTerminationMeet();
        virtual void ActionAfterRun() = 0;
    };

    template <class T, template <typename> class TSolution>
    void EvolutionaryAlgorithm<T, TSolution>::SetObjectiveSize(int obj_size)
    {
        if (m_objective_names.size() != 0 && m_objective_names.size() != obj_size)
        {
            throw("objnames_size you have set should be equal to obj_size sent in@" + __FUNCTION__);
        }
        else
        {
            m_objective_size = obj_size;
        }
    }

    template <class T, template <typename> class TSolution>
    void EvolutionaryAlgorithm<T, TSolution>::SetObjectiveNames(const std::vector<std::string> &objective_names)
    {
        if (m_objective_size == 0)
        {
            m_objective_names = objective_names;
            m_objective_size = objective_names.size();
        }
        else if (m_objective_names.size() == m_objective_size)
        {
            m_objective_names = objective_names;
        }
        else
        {
            throw(std::string("objs_size you have set should be equal to objective_names.size() sent in@") + __FUNCTION__);
        }
    }

    template <class T, template <typename> class TSolution>
    std::vector<float> EvolutionaryAlgorithm<T, TSolution>::GetLastObjsAverage() const
    {
        std::vector<float> last_obj_aver(m_objective_size);
        for (int i = 0; i < m_objective_size; ++i)
        {
            last_obj_aver[i] = m_history_objs_ave[i].back();
        }
        return last_obj_aver;
    }

    template <class T, template <typename> class TSolution>
    std::vector<float> EvolutionaryAlgorithm<T, TSolution>::GetLastObjsBest() const
    {
        std::vector<float> last_obj_best(m_objective_size);
        for (int i = 0; i < m_objective_size; ++i)
        {
            last_obj_best[i] = m_history_objs_best[i].back();
        }
        return last_obj_best;
    }

    template <class T, template <typename> class TSolution>
    std::vector<float> EvolutionaryAlgorithm<T, TSolution>::GetLastObjsWorst() const
    {
        std::vector<float> last_obj_worst(m_objective_size);
        for (int i = 0; i < m_objective_size; ++i)
        {
            last_obj_worst[i] = m_history_objs_worst[i].back();
        }
        return last_obj_worst;
    }

    template <class T, template <typename> class TSolution>
    std::shared_ptr<TerminatorBase<T, TSolution>> EvolutionaryAlgorithm<T, TSolution>::TerminationCondition(TERMINATION_CONDITION type)
    {
        return m_termination_conditions[type];
    }

    template <class T, template <typename> class TSolution>
    void EvolutionaryAlgorithm<T, TSolution>::InitBeforeRun()
    {
        for (Population &pop : m_populations)
        {
            pop.clear();
            pop.resize(m_population_size); // they have the same size
            for (TSolution<T> &sol : pop)  // they have the same evaluation method
            {
                sol.objectives.resize(m_objective_size);
            }
        }

        m_current_generation = 0;
        m_history_objs.clear();
        m_history_objs_ave.resize(m_objective_size);
        m_history_objs_best.resize(m_objective_size);
        m_history_objs_worst.resize(m_objective_size);
        for (size_t i = 0; i < m_objective_size; i++)
        {
            m_history_objs_ave[i].clear();
            m_history_objs_best[i].clear();
            m_history_objs_worst[i].clear();
        }

        std::dynamic_pointer_cast<CT>(m_termination_conditions[TERMINATION_CONDITION::CONVERGENCE])->clear();
#ifdef USE_GRAPHICS
        m_overall_evolution_status_renderer.SetXRange(0, std::dynamic_pointer_cast<MGT>(m_termination_conditions[TERMINATION_CONDITION::MAX_GENERATION]).MaxGeneration());
#endif // USE_GRAPHICS
    }

    template <class T, template <typename> class TSolution>
    std::vector<TSolution<T>> EvolutionaryAlgorithm<T, TSolution>::Run()
    {
        InitBeforeRun();
        Generate(); // 各自generate各自
        Evaluate(); //todo 改掉
        // Select(); // no select here, the whole population need remaining
        ActionAfterEachGeneration(); // you need to run it after the first generation
        for (m_current_generation = 1; !CheckIfTerminationMeet(); ++m_current_generation)
        {
            Breed();    //todo breed之后立马连接
            Evaluate(); // evaluate them by input sol of both sides into one simulator
            Select();   // Select each
            ActionAfterEachGeneration();
        }
        ActionAfterRun();
        typename std::vector<TSolution<T>>::iterator end_it = m_populations[0].begin(); // 显式指明这是一个类型 // only the main pop need to be returned
        for (end_it = m_populations[0].begin(); end_it != m_populations[0].end(); ++end_it)
        {
            if ((*end_it).rank > (*m_populations[0].begin()).rank)
            {
                break;
            }
        }
        std::cout << "Finish run after " << m_current_generation - 1 << " generations!@" << __FUNCTION__ << std::endl;
        return std::vector<TSolution<T>>(m_populations[0].begin(), end_it);
    }

    template <class T, template <typename> class TSolution>
    void EvolutionaryAlgorithm<T, TSolution>::ActionAfterEachGeneration()
    {
        RecordRunningData();
        OutputToConsoleEachGeneration(std::cout);
#ifdef USE_GRAPHICS
        ShowGraphEachGeneration();
#endif // USE_GRAPHICS
        return;
    }

    template <class T, template <typename> class TSolution>
    void EvolutionaryAlgorithm<T, TSolution>::RecordRunningData()
    {
        RecordObjectives();
    }

    template <class T, template <typename> class TSolution>
    void EvolutionaryAlgorithm<T, TSolution>::RecordObjectives() //! only the main pop need to be recorded
    {
        // all the objs
        int pop_sz = m_populations[0].size();
        m_history_objs.emplace_back(std::vector<std::vector<float>>(pop_sz, std::vector<float>(m_objective_size)));
        std::vector<std::vector<float>> &current_generation_objs = m_history_objs.back();
        for (size_t i = 0; i < pop_sz; ++i)
        {
            current_generation_objs[i] = m_populations[0][i].objectives;
        }
        for (size_t i = 0; i < m_objective_size; ++i)
        {
            // ave
            m_history_objs_ave[i].push_back(std::accumulate(current_generation_objs.begin(), current_generation_objs.end(), 0.f, [i](float initial_value, const std::vector<float> &so) -> float {
                return initial_value + so[i];
            }));
            m_history_objs_ave[i].back() /= pop_sz;
            // best
            auto best_iter_i = std::max_element(current_generation_objs.begin(), current_generation_objs.end(), [i](const std::vector<float> &so1, const std::vector<float> so2) -> bool {
                return so1[i] < so2[i];
            });
            m_history_objs_best[i].push_back((*best_iter_i)[i]);
            // worst
            auto worst_iter_i = std::min_element(current_generation_objs.begin(), current_generation_objs.end(), [i](const std::vector<float> &so1, const std::vector<float> &so2) -> bool {
                return so1[i] < so2[i];
            });
            m_history_objs_worst[i].push_back((*worst_iter_i)[i]);
        }
    }

    template <class T, template <typename> class TSolution>
    void EvolutionaryAlgorithm<T, TSolution>::RecordObjectives_(int pop_index) //! only the main pop need to be recorded
    {
        // all the objs
        int pop_sz = m_populations[pop_index].size();
        m_history_objs.emplace_back(std::vector<std::vector<float>>(pop_sz, std::vector<float>(m_objective_size)));
        std::vector<std::vector<float>> &current_generation_objs = m_history_objs.back();
        for (size_t i = 0; i < pop_sz; ++i)
        {
            current_generation_objs[i] = m_populations[pop_index][i].objectives;
        }
        for (size_t i = 0; i < m_objective_size; ++i)
        {
            // ave
            m_history_objs_ave[i].push_back(std::accumulate(current_generation_objs.begin(), current_generation_objs.end(), 0.f, [i](float initial_value, const std::vector<float> &so) -> float {
                return initial_value + so[i];
            }));
            m_history_objs_ave[i].back() /= pop_sz;
            // best
            auto best_iter_i = std::max_element(current_generation_objs.begin(), current_generation_objs.end(), [i](const std::vector<float> &so1, const std::vector<float> so2) -> bool {
                return so1[i] < so2[i];
            });
            m_history_objs_best[i].push_back((*best_iter_i)[i]);
            // worst
            auto worst_iter_i = std::min_element(current_generation_objs.begin(), current_generation_objs.end(), [i](const std::vector<float> &so1, const std::vector<float> &so2) -> bool {
                return so1[i] < so2[i];
            });
            m_history_objs_worst[i].push_back((*worst_iter_i)[i]);
        }
    }

    template <class T, template <typename> class TSolution>
    void EvolutionaryAlgorithm<T, TSolution>::OutputToConsoleEachGeneration(std::ostream &out)
    {
        OutputCurrentObjectives(out);
    }

    template <class T, template <typename> class TSolution>
    void EvolutionaryAlgorithm<T, TSolution>::OutputCurrentObjectives(std::ostream &out)
    {
        std::vector<std::string> field_names(m_objective_size * 2);
        for (size_t i = 0; i < m_objective_size; ++i)
        {
            field_names[0 + i] = m_objective_names[i] + " average";
            out << field_names[0 + i] << ": \t" << m_history_objs_ave[i].back() << "\t";
            field_names[m_objective_size + i] = m_objective_names[i] + " best";
            out << field_names[m_objective_size + i] << ": \t" << m_history_objs_best[i].back() << "\t";
            // objective_names[m_objective_size * 2 + i] = m_objective_names[i] + " worst";
        }
        out << std::endl;
    }

    template <class T, template <typename> class TSolution>
    void EvolutionaryAlgorithm<T, TSolution>::OutputAllHistoryObjectives(std::ostream &out)
    {
        std::vector<std::string> field_names(m_objective_size * 2);
        size_t history_length = m_history_objs_ave.front().size();
        for (int j = 0; j < history_length; ++j)
        {
            for (size_t i = 0; i < m_objective_size; ++i)
            {
                field_names[0 + i] = m_objective_names[i] + " average";
                out << field_names[0 + i] << ": \t" << m_history_objs_ave[i][j] << "\t";
                field_names[m_objective_size + i] = m_objective_names[i] + " best";
                out << field_names[m_objective_size + i] << ": \t" << m_history_objs_best[i][j] << "\t";
                // objective_names[m_objective_size * 2 + i] = m_objective_names[i] + " worst";
            }
            out << std::endl;
        }
        out << std::endl; // split diffrent run
    }

#ifdef USE_GRAPHICS
    template <class T, template <typename> class TSolution>
    void EvolutionaryAlgorithm<T, TSolution>::ShowGraphEachGeneration()
    {
        //todo show the evolution status of the algorithm
        ShowOverallStatusGraphEachGeneration();
        ShowSolutionDistribution(3); // show the last generations objs distribution
    }

    template <class T, template <typename> class TSolution>
    void EvolutionaryAlgorithm<T, TSolution>::ShowOverallStatusGraphEachGeneration()
    {
        std::vector<std::vector<float>> data;
        data.reserve(2 * 3 * m_objective_size);                                          // I don't know if it is useful
        data.insert(data.end(), m_history_objs_ave.begin(), m_history_objs_ave.end());   // insert objective_size's vectors in data
        data.insert(data.end(), m_history_objs_best.begin(), m_history_objs_best.end()); // insert objective_size's vectors in data
        // data.insert(data.end(), m_history_objs_worst.begin(), m_history_objs_worst.end()); // insert objective_size's vectors in data

        std::vector<float> generation_indices(m_current_generation + 1);
        std::iota(generation_indices.begin(), generation_indices.end(), 0);
        std::vector<std::string> line_names(m_objective_size * 2);
        // names order is obj1_ave, obj2_ave,.. objN_ave, obj1_best, ..., obj1_worst ,objN_worst
        for (size_t i = 0; i < m_objective_size; ++i)
        {
            line_names[0 + i] = m_objective_names[i] + " average";
            // std::cout << line_names[0 + i] << ": \t" << m_history_objs_ave[i].back() << "\t";
            line_names[m_objective_size + i] = m_objective_names[i] + " best";
            // std::cout << line_names[m_objective_size + i] << ": \t" << m_history_objs_best[i].back() << "\t";
            // line_names[m_objective_size * 2 + i] = m_objective_names[i] + " worst";
        }
        // std::cout << std::endl;
        m_overall_evolution_status_renderer.Show(data, generation_indices, line_names);
    }
#endif // USE_GRAPHICS

    template <class T, template <typename> class TSolution>
    bool EvolutionaryAlgorithm<T, TSolution>::CheckIfTerminationMeet()
    {
        // if one of them meet, return true (to terminate)
        for (auto &item : m_termination_conditions)
        {
            if (item.second->operator()())
            {
                //? std::cout << static_cast<std::underlying_type<TERMINATION_CONDITION>::type>(item.first) << std::endl;
                //? std::cout << std::dynamic_pointer_cast<CT>(m_termination_conditions[TERMINATION_CONDITION::CONVERGENCE])->MaxNoImproveGeneration() << std::endl;
                return true;
            }
        }
        return false;
    }

} // namespace sc2

#endif //EVOLUTIONARY_ALGORITHM_H
