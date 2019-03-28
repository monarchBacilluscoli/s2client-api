#include "solution.h"

namespace sc2 {
	bool multi_smaller(const solution& a, const solution& b) {
		bool equal = true;
		// If any of the b's dimensions is bigger than a, return false
		for (size_t i = 0; i < a.objectives.size(); i++) {
			if (a.objectives[i] < b.objectives[i]) {
				return false;
			}
			else if (equal && fabsf(a.objectives[i] - b.objectives[i]) > 0.000001) {
				equal = false;
			}
		}
		// If all the demensions are equal, return false
		if (equal) {
			return false;
		}
		// else return true
		return true;
	}

	// Simply add up all the objectives without considering weights
	bool simple_sum_smaller(const solution& a, const solution& b) {
		return std::accumulate(a.objectives.begin(), a.objectives.end(), 0) < std::accumulate(b.objectives.begin(), b.objectives.end(), 0);
	}
}

