#ifndef ROLLING_BOT_H
#define ROLLING_BOT_H

#include <queue>
#include <string>
#include <sc2api/sc2_api.h>
#include <functional>
#include <iostream>
#include "../algorithm/rolling_ga.h"
#include "../simulator/simulator.h"
#include "../algorithm/rolling_de.h"

namespace sc2
{

enum class PLAY_STYLE
{
    NORMAL = 0,
    IRONHEAD = 1, // most aggressive
    AGGRESSIVE = 2,
    DEFENSIVE = 3,
    RUNAWAY = 4 // most defensive
};

extern const std::vector<std::string> g_play_style_names; // a vector used for transforming enum to string

class RollingBot : public Agent
{
private:
    int m_interval_size = 160; //! Number of frames for which the algorithm should run once // about 5 seconds
    std::function<const RollingSolution<Command> &(const Population &)> m_solution_selector = &RollingBot::SelectMostOKSolution;

    //! The algorithm object
    RollingDE m_rolling_ea;

    // data
    std::map<Tag, std::deque<ActionRaw>> m_selected_commands;
    std::map<Tag, float> m_my_team_cooldown_last_frame;
    Units m_my_initial_team;

    // mics
    int m_game_round = 10;
    int m_current_round = 1;
    std::string m_remark;
    bool m_is_only_start = false;
    bool m_is_output_conver = false; // only output the algorithm's data at the start frame
    bool m_is_output_sim_record = false;

public:
    RollingBot() = delete;
    //! the only thing this constructor needs to do is to provid all parameters
    //! the simulator needs You need to ensure that all the settings are valid
    //! in remote client, especially the map_path (a lot of errors have happend
    //! to it)
    RollingBot(const std::string &net_address, int port_start,
               const std::string &process_path, const std::string &map_path, int max_generation = 50, int population_size = 50, bool use_enemy_pop = false)
        : m_rolling_ea(net_address, port_start, process_path, map_path, max_generation, population_size) {}
    virtual void OnGameStart() override;
    virtual void OnStep() override;
    virtual void OnUnitDestroyed(const Unit *u) override;
    virtual void OnGameEnd() override;
    // Settings for bot
    void SetIntervalLength(int frames);
    void SetStyle(PLAY_STYLE style);

    void SetGameRound(int round);
    void SetRemark(const std::string &remark);
    void SetOnlyStart(bool only) { m_is_only_start = only; };
    bool GetOnlyStart() const { return m_is_only_start; };
    void SetOutputConver(bool use) { m_is_output_conver = use; };
    bool GetOutputConver() const { return m_is_output_conver; };
    void SetOutputSimRecord(bool use) { m_is_output_sim_record = use; };
    bool GetOutputSimRecord() const { return m_is_output_sim_record; };

public:
    RollingEA &Algorithm(); // return the reference of algo obj of base class
protected:
    void SetCommandFromAlgorithm();

protected:
    static const RollingSolution<Command> &SelectMostIronHeadSolution(const Population &pop);
    static const RollingSolution<Command> &SelectMostRunAwaySolution(const Population &pop);
    static const RollingSolution<Command> &SelectMostOKSolution(const Population &pop); // forgive my small vocabulary...I mean I can get a more moderate solution that can be used at most conditions
};
} // namespace sc2

#endif // !ROLLING_BOT_H