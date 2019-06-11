#ifndef REALGA_H
#define REALGA_H

#include"GA.h"

class RealGA:public GA<float> {
    using Population = std::vector<Solution<float>>;
    using Evaluator = std::function<float(const std::vector<float>&)>;
    using Compare = std::function<bool(const Solution<float>&, const Solution<float>&)>;
public:
    RealGA() = default;
    RealGA(const std::vector<float>& lower, const std::vector<float>& upper): m_lower(lower),m_upper(upper) {
        assert(lower.size() == upper.size());
        m_dimensions = lower.size();
        for (size_t i = 0; i < m_dimensions; i++)
        {
            assert(m_lower[i] <= m_upper[i]);
        }
    }
    ~RealGA() = default;

private:
    virtual Solution<float> GenerateSolution() override;
    virtual void Mutate(Solution<float>& s) override;

    //! Algorithm settings
    std::vector<float> m_lower;
    std::vector<float> m_upper;
    int m_dimensions;
    //! Algorithm data
};

#endif // !REALGA_H
