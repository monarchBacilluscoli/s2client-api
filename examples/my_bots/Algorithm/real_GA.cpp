#include "real_GA.h"

Solution<float> RealGA::GenerateSolution()
{
	Solution<float> s(m_dimensions, m_evaluators.size());
	//todo for each dims, according to the boundary to generate the number.
	for (size_t i = 0; i < m_dimensions; i++)
	{
		s.variable[i] = sc2::GetRandomFraction()* (m_upper[i] - m_lower[i]) + m_lower[i];
	}
	return s;
}

void RealGA::Mutate(Solution<float>& s)
{
	//todo randomly select a dimension and add or sub a small number
	int index = sc2::GetRandomInteger(0, m_dimensions - 1);
	s.variable[index] += (m_upper[index] - m_lower[index]) / 10000.f;
}

