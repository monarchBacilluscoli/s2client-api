#include "rolling_de.h"

namespace sc2
{
void RollingDE::InitBeforeRun()
{
    RollingEA::InitBeforeRun();
    RollingDE::InitBeforeRun(); // I think it is ok to call the EvolutionaryAlgorithm::InitBeforeRun() twice
    //todo Initialization only for this class
}

Solution<Command> RollingDE::Mutate(const Solution<Command> &base_sol, const Solution<Command> &material_sol1, const Solution<Command> &material_sol2)
{
    Solution<Command> product(base_sol.variable);
    // generally all the solution's varible sizes are the same, perhaps I can omit the assertion
    int variable_size = std::min({product.variable.size(),
                                  material_sol1.variable.size(), material_sol2.variable.size()});
    if (variable_size == 0)
    {
        return Solution<Command>();
    }
    for (size_t i = 0; i < variable_size; ++i)
    {
        const RawActions &unit_actions_m1 = material_sol1.variable[i].actions;
        const RawActions &unit_actions_m2 = material_sol2.variable[i].actions;
        RawActions &unit_actions_p = product.variable[i].actions;
        if (base_sol.variable[i].unit_tag != material_sol1.variable[i].unit_tag || base_sol.variable[i].unit_tag != material_sol2.variable[i].unit_tag)
        {
            throw("The commands will be crossed don't belong to one unit@" + std::string(__FUNCTION__));
            //todo handle the exception
        }
        int command_size = std::min({unit_actions_p.size(), unit_actions_m1.size(), unit_actions_m2.size()});
        if (command_size == 0)
        {
            continue;
        }
        for (size_t j = 0; j < command_size; ++j)
        {
            const ActionRaw &action_m1 = unit_actions_m1[j];
            const ActionRaw &action_m2 = unit_actions_m2[j];
            ActionRaw &action_p = unit_actions_p[j];
            if (action_p.ability_id == action_m1.ability_id && action_p.ability_id == action_m2.ability_id)
            {
                if (action_p.target_type == ActionRaw::TargetType::TargetPosition && action_m1.target_type == ActionRaw::TargetType::TargetPosition && action_m2.target_type == ActionRaw::TargetType::TargetPosition)
                {
                    Vector2D difference = action_m1.target_point - action_m2.target_point;
                    action_p.target_point = action_p.target_point + m_scale_factor * (difference);
                }
            }
        }
    }
    return product;
}

void RollingDE::Crossover(const Solution<Command> &parent, Solution<Command> &child)
{
    //? I can not come up with a way to generate the 'must inheritate' index
    int variable_size = std::min(parent.variable.size(), child.variable.size());
    if (variable_size == 0)
    {
        return;
    }
    for (size_t i = 0; i < variable_size; ++i)
    {
        int command_size = std::min(parent.variable[i].actions.size(), child.variable[i].actions.size());
        if (command_size == 0)
        {
            continue;
        }
        RawActions &actions_child = child.variable[i].actions;
        const RawActions &actions_parent = parent.variable[i].actions;
        for (size_t j = 0; j < command_size; j++)
        {
            //if crossover, means the child's current action don't need to be changed, since it has been the mutated action
            if (GetRandomFraction() > m_crossover_rate)
            {
                actions_child[j] = actions_parent[j];
            }
        }
    }
}
} // namespace sc2