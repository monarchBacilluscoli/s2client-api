#ifndef SOLUTION_H
#define SOLUTION_H

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

#endif //SOLUTION_H