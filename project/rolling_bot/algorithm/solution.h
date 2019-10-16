#ifndef SOLUTION_H
#define SOLUTION_H

#include<limits>
#include<list>

enum class DOMINANCE{
    BETTER,
    WORSE,
    EQUAL
};

template<class T>
struct Solution
{
    std::vector<float> objectives = std::vector<float>();
    std::vector<T> variable = std::vector<T>();
    int rank = std::numeric_limits<int>::max(); // I haven't used it yet
    int dominated_count = 0;
    std::list<Solution<T>*> dominant_solutions;

    bool operator==(const Solution& rhs) const {
        return variable == rhs.variable;
    }
    bool operator!=(const Solution& rhs) const {
        return !(*this == rhs);
    }

    Solution<T>() = default;
    Solution<T>(const std::vector<T>& rhs):objectives(rhs.objectives),variable(rhs.variable) {}
    Solution<T>(int variable_size, int objective_size):variable(variable_size), objectives(objective_size) {}
    Solution<T>(int variable_size):variable(variable_size),objectives(1) {}

    //! two compares for the use of sort(), descending order 
    // return true means the orders of the two items keep unchanged
    static bool multi_greater(const Solution<T>& a, const Solution<T>& b);
    static bool sum_greater(const Solution<T>& a, const Solution<T>& b);
    static DOMINANCE Dominate(const Solution<T>& a, const Solution<T>& b);
};

#endif //SOLUTION_H