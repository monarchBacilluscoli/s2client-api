#ifndef REAL_GA_H
#define REAL_GA_H

#include "ga.h"

class RealGA : public GA<float>
{
public:
    using Population = std::vector<Solution<float>>;
    using Evaluator = std::function<float(const std::vector<float> &)>;
    using Compare = std::function<bool(const Solution<float> &, const Solution<float> &)>;

public:
    RealGA() = default;
    RealGA(const std::vector<float> &lower, const std::vector<float> &upper) : m_lower(lower), m_upper(upper)
    {
        assert(lower.size() == upper.size());
        m_dimensions = lower.size();
        for (size_t i = 0; i < m_dimensions; i++)
        {
            assert(m_lower[i] <= m_upper[i]);
        }
    }
    ~RealGA() = default;

    void SetBoundry(const std::vector<float> &lower, const std::vector<float> &upper);

private:
    virtual Solution<float> GenerateSolution() override;
    virtual void Mutate(Solution<float> &s) override;
    //! Algorithm settings
    std::vector<float> m_lower;
    std::vector<float> m_upper;
    int m_dimensions;

public:
    // set dimensions for examle evaluator and automatically set boundry for each dim
    static void SetExampleDimensions(int dimensions);
    //! an example evaluator for test
    static Evaluator example_evaluator;
    static std::vector<float> example_lower_boundry;
    static std::vector<float> example_upper_boundry;
};

#endif // REAL_GA_H