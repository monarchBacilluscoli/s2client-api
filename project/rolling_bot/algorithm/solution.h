#ifndef SOLUTION_H
#define SOLUTION_H

#include <limits>
#include <list>
#include <vector>
#include <numeric>
#include <algorithm>

enum class DOMINANCE
{
    BETTER,
    WORSE,
    EQUAL
};

template <class T>
struct Solution
{
    using Population = std::vector<Solution<T>>;

    std::vector<float> objectives = std::vector<float>();
    std::vector<T> variable = std::vector<T>();
    int rank = std::numeric_limits<int>::max(); // I haven't used it yet
    int dominated_count = 0;
    std::list<Solution<T> *> dominant_solutions;

    bool operator==(const Solution &rhs) const
    {
        return variable == rhs.variable;
    }
    bool operator!=(const Solution &rhs) const
    {
        return !(*this == rhs);
    }

    Solution<T>() = default;
    Solution<T>(const Solution<T> &rhs) : variable(rhs.variable), objectives(rhs.objectives) {} //! attention
    Solution<T>(const std::vector<T> &variable) : variable(variable) {}
    Solution<T>(int variable_size, int objective_size) : variable(variable_size), objectives(objective_size) {}
    Solution<T>(int variable_size) : variable(variable_size), objectives(1) {}

    //! two compares for the use of sort(), descending order
    // return true means the orders of the two items keep unchanged
    static bool multi_greater(const Solution<T> &a, const Solution<T> &b);
    static bool sum_greater(const Solution<T> &a, const Solution<T> &b);
    static bool RankLess(const Solution<T> &a, const Solution<T> &b); // looks like it should not be a member function of Solution. If so, it will not be used as a Compare, since it accept one more parameter - this pointer
    static DOMINANCE Dominate(const Solution<T> &a, const Solution<T> &b);
    static void DominanceSort(Population &pop);
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
        else if (equal && abs(a.objectives[i] - b.objectives[i]) > 0.000001)
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

template <class T>
bool Solution<T>::sum_greater(const Solution<T> &a, const Solution<T> &b)
{
    return std::accumulate(a.objectives.begin(), a.objectives.end(), 0.f) > std::accumulate(b.objectives.begin(), b.objectives.end(), 0.f);
}

template <class T>
DOMINANCE Solution<T>::Dominate(const Solution<T> &a, const Solution<T> &b)
{
    int better_count = 0, worse_count = 0;
    int sz = a.objectives.size();
    for (size_t i = 0; i < sz; i++)
    {
        if (abs(a.objectives[i] - b.objectives[i]) > 0.000005)
        {
            if (a.objectives[i] > b.objectives[i])
            {
                ++better_count;
            }
            else
            {
                ++worse_count;
            }
        }
        if (better_count != 0 && worse_count != 0)
        {
            return DOMINANCE::EQUAL;
        }
    }
    if (better_count == 0 && worse_count == 0)
    {
        return DOMINANCE::EQUAL;
    }
    else
    {
        if (better_count)
        {
            return DOMINANCE::BETTER;
        }
        else
        {
            return DOMINANCE::WORSE;
        }
    }
}

template <class T>
bool Solution<T>::RankLess(const Solution<T> &l, const Solution<T> &r)
{
    return l.rank < r.rank;
}

template <class T>
void Solution<T>::DominanceSort(Population &p)
{
    // make sure all the rank is set right
    for (auto &item : p)
    {
        item.rank = std::numeric_limits<int>::max();
    }
    // set dominate relationship
    size_t pop_sz = p.size();
    for (size_t i = 0; i < pop_sz - 1; i++)
    {
        for (size_t j = i + 1; j < pop_sz; j++)
        {
            switch (Solution<T>::Dominate(p[i], p[j]))
            {
            case DOMINANCE::BETTER:
                p[i].dominant_solutions.push_back(&p[j]);
                p[j].dominated_count++;
                break;
            case DOMINANCE::WORSE:
                p[j].dominant_solutions.push_back(&p[i]);
                p[i].dominated_count++;
                break;
            default:
                //if p[i] and p[j] are equal, do nothing
                break;
            }
        }
    }
    // set rank and sort
    int current_set_count = 0;
    int current_rank = 0;
    while (current_set_count < pop_sz)
    {
        std::list<int> current_layer_indices;
        for (size_t i = 0; i < pop_sz; i++)
        {
            if (p[i].dominated_count == 0 && p[i].rank > current_rank)
            {
                // set rank and store the solution into current layer
                p[i].rank = current_rank;
                current_layer_indices.push_back(i);
            }
        }
        // handle the solutions dominated by solutions in current layer
        int current_layer_sz = current_layer_indices.size();
        //handle count first
        for (const auto &index : current_layer_indices)
        {
            for (auto &item : p[index].dominant_solutions)
            {
                item->dominated_count--;
            }
            p[index].dominant_solutions.clear();
        }
        current_set_count += current_layer_sz;
        ++current_rank;
    }
    std::sort(p.begin(), p.end(), &Solution<T>::RankLess);
}

#endif //SOLUTION_H