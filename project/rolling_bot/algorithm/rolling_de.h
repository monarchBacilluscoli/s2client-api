#ifndef ROLLING_DE_H
#define ROLLING_DE_H

#include "differential_evolution.h"
#include "../simulator/simulator_pool.h"
#include "../contrib/gnuplot-iostream.h"
#include "rolling_ea.h"

namespace sc2
{

    class RollingDE : public DifferentialEvolution<Command, RollingSolution>, public RollingEA
    {
    private:
    protected:
        using EA = EvolutionaryAlgorithm<Command, RollingSolution>;
        bool m_is_enemy_pop_evo = false;

    public:
        RollingDE() = delete;
        /* I must call the construct of the virtual base class, since all the virtual derived classes will not call it (since the multiple calls will cause multiple constructing, it is banned)
    So, the parameters in constructors of the direct virtual derived classes are useless.
    */
        RollingDE(const std::string &net_address,
                  int port_start,
                  const std::string &process_path,
                  const std::string &map_path,
                  int max_generation = 50,
                  int population_size = 50,
                  bool use_enemy_pop = false,
                  float scale_factor = .5f,
                  float crossover_rate = .5f,
                  int random_seed = rand(),
                  bool is_enemy_pop_evo = false) : EvolutionaryAlgorithm(3, max_generation, population_size, random_seed, {"Enemy Loss", "My Team Loss"}, use_enemy_pop ? 2 : 1),
                                                   DifferentialEvolution(3, max_generation, population_size, scale_factor, crossover_rate),
                                                   RollingEA(net_address, port_start, process_path, map_path, max_generation, population_size, use_enemy_pop, random_seed),
                                                   m_is_enemy_pop_evo(is_enemy_pop_evo)
        {
            SetAttackPossibility(.9f);
        }
        virtual ~RollingDE() = default;
        
        void SetEnemyPopEvo(bool is_enemy_pop_evo = false);

    protected:
        void InitBeforeRun() override;
        virtual void Breed() override;
        virtual RollingSolution<Command> Mutate(const RollingSolution<Command> &base_sol, const RollingSolution<Command> &material_sol1, const RollingSolution<Command> &material_sol2) override;
        virtual void Crossover(const RollingSolution<Command> &parent, RollingSolution<Command> &child) override;

        virtual void Breed_(int pop_index = 0);
        void Fix_(int pop_index);
    };
} // namespace sc2

#endif //ROLLING_DE_H