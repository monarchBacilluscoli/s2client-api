#include "sc2api/sc2_action.h"

#include <iostream>
#include <cassert>

#include "s2clientprotocol/sc2api.pb.h"

namespace sc2 {

ActionRaw::ActionRaw() :
    ability_id(0),
    target_type(TargetNone),
    target_tag(NullTag) {
}

ActionRaw::ActionRaw(AbilityID in_ability_id, TargetType in_target_type, Tag in_target_tag, Point2D in_target_point) :
    ability_id(in_ability_id),
    target_type(in_target_type),
    target_tag(in_target_tag),
    target_point(in_target_point) {
}

}