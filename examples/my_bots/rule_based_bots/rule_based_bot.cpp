#include "rule_based_bot.h"

namespace sc2 {
	const Unit* sc2::rule_based_bot::select_nearest_unit_from_point(const Point2D& p, const Units& us) {
		float min_distance = FLT_MAX;
		const Unit* selected_unit = nullptr;
		float dis;
		for (const auto u : us) {
			dis = Distance2D(p, u->pos);
			if (dis < min_distance) {
				selected_unit = u;
				min_distance = dis;
			}
		}
		return selected_unit;
	}
}

