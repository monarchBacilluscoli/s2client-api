// terminator is used for checking if the algorithm can be ended

#ifndef TERMINATOR_H
#define TERMINATOR_H

#include <vector>
#include "./evolutionary_algorithm.h"

namespace sc2
{
template <class T, template <typename> class TSolution>
class EvolutionaryAlgorithm;

template <class T, template <typename> class TSolution>
class TerminatorBase
{
protected:
    using EA = EvolutionaryAlgorithm<T, TSolution>;
    const EvolutionaryAlgorithm<T, TSolution> &m_algorithm;

public:
    TerminatorBase(const EA &algorithm) : m_algorithm(algorithm) {}
    virtual bool operator()() = 0;
};

template <class T, template <typename> class TSolution>
class MaxGenerationTerminator : public TerminatorBase<T, TSolution>
{
private:
    using EA = EvolutionaryAlgorithm<T, TSolution>;
    using TB = TerminatorBase<T, TSolution>;
    int m_max_generation = 10;

public:
    MaxGenerationTerminator(const EA &algorithm) : TB(algorithm){};
    MaxGenerationTerminator(const EA &algorithm, int max_generation) : TB(algorithm), m_max_generation(max_generation)
    {
        if (m_max_generation < 0)
        {
            throw(std::string("you need to set a positive value for max_generation@") + __FUNCTION__);
        }
    };
    virtual bool operator()() { return TB::m_algorithm.GetCurrentGeneration() > m_max_generation; }

    int MaxGeneration() const { return m_max_generation; };
    void SetMaxGeneration(int max_generation);
};

template <class T, template <typename> class TSolution>
class MaxEvaluationTerminator : public TerminatorBase<T, TSolution>
{
private:
    using EA = EvolutionaryAlgorithm<T, TSolution>;
    using TB = TerminatorBase<T, TSolution>;
    int m_max_evaluation = 100;

public:
    MaxEvaluationTerminator(const EA &algorithm) : TB(algorithm){};
    MaxEvaluationTerminator(const EA &algorithm, int max_evaluation) : TB(algorithm), m_max_evaluation(max_evaluation)
    {
        if (m_max_evaluation < 0)
        {
            throw(std::string("you need to set a positive value for max_generation@") + __FUNCTION__);
        }
    }
    virtual bool operator()() { return TB::m_algorithm.CurrentEvaluation() > m_max_evaluation; };

    int MaxEvaluation() const { return m_max_evaluation; };
    void SetMaxEvaluation(int max_evalution);
};

template <class T, template <typename> class TSolution>
class ConvergenceTerminator : public TerminatorBase<T, TSolution> // convergence termination condition checker for this class
{
private:
    using EA = EvolutionaryAlgorithm<T, TSolution>;
    using TB = TerminatorBase<T, TSolution>;
    // settings
    float m_no_improve_threshold = .00001f; //
    int m_max_no_impreve_generation = 20;
    // data
    int m_current_no_improve_generation = 0;
    std::vector<float> m_last_record_obj_average;

public:
    ConvergenceTerminator(const EA &algorithm) : TB(algorithm){};
    ConvergenceTerminator(const EA &algorithm, int max_no_improve_generation, float no_improve_tolerance) : TB(algorithm), m_max_no_impreve_generation(max_no_improve_generation), m_no_improve_threshold(no_improve_tolerance){};
    ~ConvergenceTerminator() = default;
    virtual bool operator()();

    float NoImproveTolerance() { return m_no_improve_threshold; };
    int MaxNoImproveGeneration() { return m_max_no_impreve_generation; };
    void SetNoImproveTolerance(float no_improve_tolerance) { m_no_improve_threshold = no_improve_tolerance; };
    void SetMaxNoImproveGeneration(int max_no_improve_generation) { m_max_no_impreve_generation = max_no_improve_generation; };
    void clear(); // for the use of next run
};

template <class T, template <typename> class TSolution>
void MaxGenerationTerminator<T, TSolution>::SetMaxGeneration(int max_generation)
{
    if (max_generation < 0)
    {
        throw(std::string("you need to set a positive value for max_generation@") + __FUNCTION__);
    }
    else
    {
        m_max_generation = max_generation;
    }
}

template <class T, template <typename> class TSolution>
void MaxEvaluationTerminator<T, TSolution>::SetMaxEvaluation(int max_evaluation)
{
    if (max_evaluation < 0)
    {
        throw(std::string("you need to set a positive value for max_evaluation@") + __FUNCTION__);
    }
    else
    {
        m_max_evaluation = max_evaluation;
    }
}

template <class T, template <typename> class TSolution>
bool ConvergenceTerminator<T, TSolution>::operator()()
{
    std::vector<float> current_averages = TB::m_algorithm.GetLastObjsAverage();
    if (m_last_record_obj_average.empty()) // the first time to check
    {
        m_last_record_obj_average = current_averages;
        return false;
    }
    float current_difference = current_averages[0] - current_averages[1];
    float last_difference = m_last_record_obj_average[0] - m_last_record_obj_average[1];
    m_last_record_obj_average = current_averages;
    if (std::abs(current_difference - last_difference) > m_no_improve_threshold) // improve from last generation
    {
        m_current_no_improve_generation = 0;
        return false;
    }
    else if (++m_current_no_improve_generation < m_max_no_impreve_generation) // no improvement from last generation
    {
        return false;
    }
    else
    {
        return true;
    }
}

template <class T, template <typename> class TSolution>
void ConvergenceTerminator<T, TSolution>::clear()
{
    m_current_no_improve_generation = 0;
    m_last_record_obj_average.clear();
}

} // namespace sc2

#endif // TERMINATOR_H
