#include "rolling_de.h"
#include <sc2lib/sc2_utils.h>

namespace sc2
{

    void RollingDE::InitBeforeRun()
    {
        EvolutionaryAlgorithm::InitBeforeRun();
        DifferentialEvolution<Command, RollingSolution>::InitOnlySelfMembersBeforeRun(); // I think it is ok to call the EvolutionaryAlgorithm::InitBeforeRun() twice
        RollingEA::InitOnlySelfMembersBeforeRun();
        //todo Initialization only for this class
    }

    void RollingDE::Breed()
    {
        if (m_use_fix_by_data && m_current_generation % 6 == 0) // if use fix, then fix all individuals based on the sim data
        {
            std::cout << "fix" << std::endl;
            for (int l = 0; l < m_populations.size(); l++)
            {
                int sz = EA::m_populations[l].size();
                EA::m_offsprings[l] = EA::m_populations[l];
                for (int i = 0; i < sz; ++i)
                {
                    m_offsprings[l][i] = FixBasedOnSimulation(m_populations[l][i]);
                }
            }
        }
        else
        {
            DifferentialEvolution<Command, RollingSolution>::Breed();
        }
        if (m_use_assemble) //? nothing wrong, use the population's evaluation to make a new solution, and replace a random solution in offspring. It's ok
        {
            RollingEA::AssembleASolutionFromGoodUnits(GetRandomEntry(m_offsprings[0]), m_populations[0]); // randomly exchange 1 solution with a new assemble solution
        }
    }

    RollingSolution<Command> RollingDE::Mutate(const RollingSolution<Command> &base_sol, const RollingSolution<Command> &material_sol1, const RollingSolution<Command> &material_sol2)
    {
        RollingSolution<Command> product(base_sol.variable);
        // generally all the solution's varible sizes are the same, perhaps I can omit the assertion
        int variable_size = std::min({product.variable.size(),
                                      material_sol1.variable.size(), material_sol2.variable.size()});
        if (variable_size == 0)
        {
            return RollingSolution<Command>();
        }
        for (size_t i = 0; i < variable_size; ++i) // for each unit
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
                if (/*action_p.ability_id == action_m1.ability_id && action_p.ability_id == action_m2.ability_id*/ true)
                {
                    if (action_p.target_type == ActionRaw::TargetType::TargetPosition && action_m1.target_type == ActionRaw::TargetType::TargetPosition && action_m2.target_type == ActionRaw::TargetType::TargetPosition)
                    {
                        Vector2D difference = action_m1.target_point - action_m2.target_point;
                        action_p.target_point = action_p.target_point + m_scale_factor * (difference);
                        action_p.target_point = FixOutsidePointIntoRectangle(action_p.target_point, m_game_info.playable_min, m_game_info.playable_max);
                        if (m_use_fix)
                        {
                            if (GetRandomFraction() < 0.7)
                            {
                                action_p.target_point = FixActionPosIntoEffectiveRangeToNearestEnemy(action_p.target_point, m_unit_types[m_my_team[i]->unit_type].weapons.front().range, m_enemy_team);
                            }
                        }
                    }
                }
            }
        }
        return product;
    }

    void RollingDE::Crossover(const RollingSolution<Command> &parent, RollingSolution<Command> &child)
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
                if (GetRandomFraction() > m_crossover_rate)
                {
                    actions_child[j] = actions_parent[j];
                }
            }
#if 0  //! test code \
    //todo output child actions
        for (const auto &item : actions_child)
        {
            std::cout << "(" << item.target_point.x << "," << item.target_point.y << ")"
                      << "\t";
        }
        std::cout << std::endl;
        //todo output parent actions
        for (const auto &item : actions_parent)
        {
            std::cout << "(" << item.target_point.x << "," << item.target_point.y << ")"
                      << "\t";
        }
        std::cout << std::endl;
#endif //! test code>
        }
    }

} // namespace sc2