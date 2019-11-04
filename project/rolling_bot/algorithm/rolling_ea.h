#ifndef ROLLING_EA_H
#define ROLLING_EA_H

#include "evolutionary_algorithm.h"
#include "../simulator/simulator_pool.h"

namespace sc2
{
class RollingEA : virtual public EvolutionaryAlgorithm<Command> //! because of virtual inheritance, the base class's constructor is invalid
{
public:
    using EA = EvolutionaryAlgorithm<Command>;

protected:
    // game data
    const ObservationInterface *m_observation;
    Units m_my_team;
    Units m_enemy_team;
    GameInfo m_game_info;
    Vector2D m_playable_dis;
    // settings about game
    float m_attack_possibility = 0.7;
    int m_command_length = 8;
    int m_run_length = 300;
    // methods
    ScatterRenderer2D m_objective_distribution;
    DebugRenderers m_debug_renderers;
    bool m_is_debug = true;
    // simulators
    SimulatorPool m_simulation_pool;

public:
    RollingEA() = delete;
    RollingEA(const std::string &net_address, int port_start, const std::string &process_path, const std::string &map_path, int max_generation, int population_size, int random_seed = 0) : EvolutionaryAlgorithm(2, max_generation, population_size, random_seed, {std::string("enemy loss"), std::string("my team loss")}), m_debug_renderers(population_size), m_simulation_pool(population_size, net_address, port_start, process_path, map_path)
    {
        m_simulation_pool.StartSimsAsync();
        m_objective_distribution.SetTitle("Objectives Distribution");
        m_objective_distribution.SetXLabel("total damage to enemy");
        m_objective_distribution.SetYLabel("total damage to me");
        SetObjectiveNames({"Enemy Loss", "My Team Loss"});
    }
    virtual ~RollingEA() = default;

    void Initialize(const ObservationInterface *observation);

public:
    // settings about the game
    void SetRunLength(int run_length) { m_run_length = run_length; }
    void SetCommandLength(int command_length) { m_command_length = command_length; }
    void SetAttackPossibility(float attack_possibility) { m_attack_possibility = attack_possibility; }
    void SetDebug(bool is_debug) { m_is_debug = is_debug; }

protected:
    // override functions
    virtual void InitBeforeRun() override;
    void InitOnlySelfMembersBeforeRun();
    void Generate() override;
    void Evaluate() override;
    void Select() override;
    virtual void ShowSolutionDistribution(int showed_generations_count) override;
    //! for test
    virtual void ActionAfterEachGeneration() override
    {
        // float self_loss = 0, self_team_loss_total = 0, self_team_loss_best = std::numeric_limits<float>::max();
        // float enemy_loss = 0, enemy_team_loss_total = 0, enemy_team_loss_best = std::numeric_limits<float>::lowest();
        // for (size_t i = 0; i < EA::m_population_size; ++i)
        // {
        //     self_team_loss_total += self_loss;
        //     enemy_team_loss_total += enemy_loss;
        //     self_team_loss_best = self_team_loss_best < self_loss ? self_team_loss_best : self_loss;
        //     enemy_team_loss_best = enemy_team_loss_best > enemy_loss ? enemy_team_loss_best : enemy_loss;
        // }
        // self_team_loss_total = std::accumulate(EA::m_population.begin(), EA::m_population.end(), 0,
        //                                        [](float init, const Solution<Command> &s2) -> float {
        //                                            return init + s2.objectives[0];
        //                                        });
        // std::cout
        //     << "enemy aver: "
        //     << "\t" << enemy_team_loss_total / EA::m_population_size << "\t";
        // std::cout << "enemy best: "
        //           << "\t"
        //           << enemy_team_loss_best << "\t";
        // std::cout << "self aver: "
        //           << "\t" << self_team_loss_total / EA::m_population_size << "\t";
        // std::cout << "self best: "
        //           << "\t" << self_team_loss_best << std::endl;
        EA::ActionAfterEachGeneration();
    };

protected:
    // only belong to this class
    void Evaluate(Population &pop);
    void InitFromObservation();
    void GenerateOne(Solution<Command> &sol);
    virtual void RecordObjectives() override;
};

} // namespace sc2

#endif //ROLLING_EA_H