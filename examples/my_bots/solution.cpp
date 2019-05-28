#include "solution.h"

namespace sc2 {


	bool solution::operator==(const solution & rhs) const {
		return commands == rhs.commands;
	}
	bool solution::operator!=(const solution & rhs) const {
		return !(*this == rhs);
	}
}

