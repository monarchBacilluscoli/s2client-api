#ifndef SC2UTILITY_H
#define SC2UTILITY_H

#include"sc2api/sc2_api.h"
#include<iostream>

namespace sc2 {
    class sc2utility {
    public:
        sc2utility() = delete;
        ~sc2utility() = delete;

        static float dis_from_radius(const Unit* a, const Unit* b) {
            return Distance2D(a->pos, b->pos) - a->radius - b->radius;
        }

        static Units units_can_be_attacked(const Unit* u, const ObservationInterface* ob, Unit::Alliance a = Unit::Alliance::Enemy) {
            Units units_can_be_attacked;
            // get the longest weapon of u and search units within its range
            std::vector<Weapon> weapons = ob->GetUnitTypeData()[u->unit_type].weapons;
            Weapon longest = get_longest_range_weapon_of_weapons(weapons);
            units_can_be_attacked = search_units_within_radius(u->pos, longest.range+u->radius,ob, a);
            // check if every unit of them can be attacked by each weapon (range & type)
            for (Units::iterator i = units_can_be_attacked.begin(); i < units_can_be_attacked.end(); i++) {
                if (!is_attackable(u, *i, ob)) {
                    units_can_be_attacked.erase(i);
                }
            }
            return units_can_be_attacked;
        }

        static void sort_target_units_by_damage(const Unit* u, Units& us, const UnitTypes& utd) {
            std::sort(us.begin(), us.end(), [&](const Unit * a, const Unit * b) {
                return damage_unit_to_unit(u, a, utd) > damage_unit_to_unit(u, b, utd);
            }); 
        }

        static bool is_attackable(const Unit* attacking_u, const Unit* target_u, const ObservationInterface* ob) {
            std::vector<Weapon> matched_weapons = get_matched_weapons_without_considering_distance(attacking_u, target_u,ob);
            float range = get_longest_range_weapon_of_weapons(matched_weapons).range;
            if (range > dis_from_radius(attacking_u, target_u)) {
                return true;
            }
            return false;
        }

        static std::vector<Weapon> get_matched_weapons_without_considering_distance(const Unit* attack, const Unit* target, const ObservationInterface* ob) {
            std::vector<Weapon> matched_weapons;
            for (const Weapon& w : ob->GetUnitTypeData()[attack->unit_type].weapons) {
                if (is_weapon_match_unit(w, target)) {
                    matched_weapons.push_back(w);
                }
            }
            return matched_weapons;
        }

        static Weapon get_longest_range_weapon_of_weapons(const std::vector<Weapon> ws) {
            if (!ws.empty()) {
                if (ws.size() == 1) {
                    return ws[0];
                }
                else {
                    Weapon longest_range_weapon = ws[0];
                    for (const Weapon& w : ws) {
                        if (w.range > longest_range_weapon.range) {
                            longest_range_weapon = w;
                        }
                    }
                    return longest_range_weapon;
                }
            }
            else {
                throw("no weapon here@one_frame_bot::get_longest_range_weapon_of_weapons");
            }
        }

        static Units search_units_within_radius(const Point2D& p, float r, const ObservationInterface* ob,Unit::Alliance a) {
            Units us = ob->GetUnits(a, [&p, r](const Unit & u) {
                if (Distance2D(u.pos, p) <= r + u.radius)
                    return true;
                else
                    return false;
            });
            return us;
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
            }
            return damage;
        }

        static bool is_weapon_match_unit(const Weapon& w, const Unit* u) {
            if ((int)(w.type) - 1 == u->is_flying || w.type == Weapon::TargetType::Any) {
                return true;
            }
            return false;
        }

        static const Unit* select_nearest_unit_from_point(const Point2D& p, const Units& us) {
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

        static float move_distance(const Unit* u, int frames, const UnitTypes& uts) {
            return uts[u->unit_type].movement_speed / frames_per_second * frames;
        }

        static void output_units_health_in_order(Units& units) {
            std::sort(units.begin(), units.end(), [](const Unit* a, const Unit* b)->bool {return a->health > b->health; });
            std::for_each(units.begin(), units.end(), [](const Unit* &a) {std::cout << a->health << "\t"; });
        }

        static const int frames_per_second = 16;
    };
}

#endif // !SC2UTILITY_H
