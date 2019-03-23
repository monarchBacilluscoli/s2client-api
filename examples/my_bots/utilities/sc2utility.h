#ifndef SC2UTILITY_H
#define SC2UTILITY_H

#include"sc2api/sc2_api.h"

namespace sc2 {
    class sc2utility {
    public:
        sc2utility() = delete;
        ~sc2utility() = delete;

        static float dis_from_radius(const Unit* a, const Unit* b) {
            return Distance2D(a->pos, b->pos) - a->radius - b->radius;
        }

        static float damage_unit_to_unit(const Unit* attacking_u, const Unit* target_u, const UnitTypes& types) {
            //! just consider the case that every unit only has only one weapon can be used to attack a specific enemy unit
            Weapon w = get_matched_weapons(attacking_u, target_u, types)[0];
            return damage_weapon_to_unit(w, target_u, types);
        }

        static std::vector<Weapon> get_matched_weapons(const Unit* attack, const Unit* target, const UnitTypes& types) {
            std::vector<Weapon> matched_weapons;
            for (const Weapon& w : types[attack->unit_type].weapons) {
                if (is_weapon_match_unit(w, target)) {
                    matched_weapons.push_back(w);
                }
            }
            return matched_weapons;
        }


        static float damage_weapon_to_unit(const Weapon& w, const Unit* u, const UnitTypes& types) {
            float damage = 0;
            UnitTypeData u_type = types[u->unit_type];
            if (is_weapon_match_unit(w, u)) {
                float bonus_damage = 0;
                for (auto i : w.damage_bonus) {
                    std::vector<Attribute>::iterator result = std::find(u_type.attributes.begin(), u_type.attributes.end(), i.attribute);
                    if (result != u_type.attributes.end()) {
                        bonus_damage += i.bonus;
                    }
                }
                damage += (w.damage_ - u_type.armor + bonus_damage) * w.attacks;
                return damage;
            }
            else {
                throw("this weapon doesn't match the unit@damage_weapon_to_unit");
            }
        }

        static bool is_weapon_match_unit(const Weapon& w, const Unit* u) {
            if ((int)(w.type) - 1 == u->is_flying || w.type == Weapon::TargetType::Any) {
                return true;
            }
            return false;
        }

        static const int frames_per_second = 16;
    };
}

#endif // !SC2UTILITY_H
