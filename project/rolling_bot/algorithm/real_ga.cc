#include "real_ga.h"

Solution<float> RealGA::GenerateSolution()
{
	Solution<float> s(m_dimensions, m_evaluators.size());
	// for each dims, according to the boundary to generate the number.
	for (size_t i = 0; i < m_dimensions; i++)
	{
		s.variable[i] = sc2::GetRandomFraction() * (m_upper[i] - m_lower[i]) + m_lower[i];
	}
	return s;
}

void RealGA::Mutate(Solution<float> &s)
{
	// randomly select a dimension and add or sub a small number
	int index = sc2::GetRandomInteger(0, m_dimensions - 1);
	s.variable[index] += (m_upper[index] - m_lower[index]) / 10000.f;
}

void RealGA::SetBoundry(const std::vector<float> &lower, const std::vector<float> &upper)
{
	assert(lower.size() == upper.size());
	m_dimensions = lower.size();
	for (size_t i = 0; i < m_dimensions; i++)
	{
		assert(lower[i] <= upper[i]);
	}
	m_lower = lower;
	m_upper = upper;
}

// initialize the example boundries
std::vector<float> RealGA::example_lower_boundry = {0,-1};
std::vector<float> RealGA::example_upper_boundry = {2, 3};
// initialize the example evaluator
RealGA::Evaluator RealGA::example_evaluator = [](std::vector<float> v) -> float {
	float result = 0.f;
	for (float x:v)
	{
		result -= pow(x - 1, 2);
	}
	return result + 1;
};

void RealGA::SetExampleDimensions(int dimensions){
	// set boundries
	RealGA::example_lower_boundry.resize(dimensions);
	RealGA::example_upper_boundry.resize(dimensions);
	std::for_each(RealGA::example_lower_boundry.begin(), RealGA::example_lower_boundry.end(), [](float &b) { b = 0; });
	std::for_each(RealGA::example_upper_boundry.begin(), RealGA::example_upper_boundry.end(), [](float &b) { b = 2; });
}
