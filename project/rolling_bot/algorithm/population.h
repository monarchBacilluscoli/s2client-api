#ifndef POPULATION_H
#define POPULATION_HPOPULATION_H

#include "solution.h"

template <class T, template <typename> class TSolution = Solution> // T is the variable type and TSolution is the solution type
class Population
{
private:
    std::list<std::vector<std::vector<float>>> m_history_objs{}; // for debug or record use, list:generation, outter vec:individual, inner vec:objs
    std::vector<std::vector<float>> m_history_objs_ave{};        // outter vec: objs, inner vec: generation
    std::vector<std::vector<float>> m_history_objs_best{};       // outter vec: objs, inner vec: generation
    std::vector<std::vector<float>> m_history_objs_worst{};      // outter vec: objs, inner vec: generation
    std::vector<TSolution<T>> m_solutions;
    int m_size = 0;

public:
    // Population(/* args */);
    Population(int pop_size) : m_solutions(pop_size){};
    ~Population();

    const std::vector<TSolution<T>> &GetSolutions() const { return m_solutions; };
    const TSolution<T> &operator[](int i) const { return m_solutions.at(i); };
    TSolution<T> &operator[](int i) { return m_solutions[i]; };
};

#endif // POPULATION_H