#ifndef SOLUTION_H
#define SOLUTION_H

#include <limits>
#include <list>
#include <vector>
#include <numeric>
#include <algorithm>
#include <string>
#include <functional>

enum class DOMINANCE
{
    BETTER,
    WORSE,
    EQUAL
};

template <class T>
struct Solution
{
    std::vector<float> objectives = std::vector<float>();
    std::vector<T> variable = std::vector<T>();

    int rank = std::numeric_limits<int>::max(); // used in multi-objective problem
    int dominated_count = 0;
    std::list<Solution<T> *> dominant_solutions;
    float crowdedness = 0.f;

    bool operator==(const Solution &rhs) const
    {
        return variable == rhs.variable;
    }
    bool operator!=(const Solution &rhs) const
    {
        return !(*this == rhs);
    }

    Solution() = default;
    Solution(const Solution<T> &rhs) = default;
    Solution(const std::vector<T> &variable) : variable(variable) {}
    Solution(int variable_size, int objective_size = 1) : variable(variable_size), objectives(objective_size) {}
    virtual ~Solution() = default;

    virtual void Clear();

    //! two compares for the use of sort(), descending order
    // return true means the orders of the two items keep unchanged
    static bool multi_greater(const Solution<T> &a, const Solution<T> &b);
    static bool sum_greater(const Solution<T> &a, const Solution<T> &b);
    static bool RankLess(const Solution<T> &a, const Solution<T> &b); // looks like it should not be a member function of Solution. If so, it will not be used as a Compare, since it accept one more parameter - this pointer
    static DOMINANCE Dominate(const Solution<T> &a, const Solution<T> &b);

    template <template <typename> class TSolution> // 只有使用模板，vector之中的派生类才能被传进来进行排序
    using Population = std::vector<TSolution<T>>;
    template <template <typename> class TSolution>
    static void DominanceSort(Population<TSolution> &pop, std::function<bool(const TSolution<T> &a, const TSolution<T> &b)> compare_less/* = TSolution<T>::RankLess*/);
    template <template <typename> class TSolution>
    static void CalculateCrowdedness(Population<TSolution> &pop);
};

template <class T>
void Solution<T>::Clear()
{
    objectives.clear();
    variable.clear();
    dominated_count = 0;
    dominant_solutions.clear();
    crowdedness = 0.f;
}

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
    if (l.rank != r.rank)
    {
        return l.rank < r.rank;
    }
    else
    {
        // Compare objectives
        int l_obj_sz = l.objectives.size();
        if (l_obj_sz != r.objectives.size())
        {
            throw("The obj sizes of the two solution are not the same @ Solution::" + std::string(__FUNCTION__));
        }
        else // ??
        {
            for (size_t i = 0; i < l_obj_sz; ++i)
            {
                if (l.objectives[i] != r.objectives[i])
                {
                    return l.objectives[i] < r.objectives[i];
                }
            }
            //If all the objectives are equal
            return l.objectives.back() < r.objectives.back();
        }
    }
}

template <class T>
template <template <typename> class TSolution>
void Solution<T>::DominanceSort(Population<TSolution> &p, std::function<bool(const TSolution<T> &a, const TSolution<T> &b)> compare_less)
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
            switch (TSolution<T>::Dominate(p[i], p[j])) //! 如果遵照TSolution是此处Solution的派生类的原则的话，此处使用Solution也没有问题（搁置），不过可能会重载静态函数？嗯，TSolution就用TSolution的支配排序方法吧
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
    std::sort(p.begin(), p.end(), compare_less);
    return;
}

template <class T>
template <template <typename> class TSolution>
void Solution<T>::CalculateCrowdedness(Population<TSolution> &pop)
{
    // Only after dominance sort, can it be called
    int pop_sz = pop.size();
    int obj_sz = pop.front().objectives.size();
    for (int i = 0; i < pop_sz; ++i)
    {
        // check the last and next
        if ((i - 1) < 0 || pop[i].rank != pop[(size_t)i - 1].rank ||
            (i + 1) >= pop_sz || pop[i].rank != pop[(size_t)i + 1].rank)
        {
            pop[i].crowdedness = std::numeric_limits<float>::max();
        }
        else
        {
            pop[i].crowdedness = 0.f;
            for (size_t j = 0; j < obj_sz; j++)
            {
                pop[i].crowdedness += std::abs(pop[(size_t)i - 1].objectives[j] - pop[(size_t)i + 1].objectives[j]);
            }
        }
    }
}

#endif //SOLUTION_H