#ifndef STATISTICAL_DATA_H
#define STATISTICAL_DATA_H

namespace sc2
{
struct UnitStatisticalData
{
    int action_number = 0;
    int attack_number = 0; // can only record the data of self units
    float health_change = 0.f;
};
} // namespace sc2

#endif