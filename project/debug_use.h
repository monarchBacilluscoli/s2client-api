// This file is only used for debugging, and will not exist in program in future

#ifdef DEBUG

#ifndef DEBUG_USE_H
#define DEBUG_USE_H

#include <iostream>

#include "rolling_bot/simulator/simulator_pool.h"

namespace sc2
{
void CheckUnitTagSame(const std::vector<Solution<Command>> &pop, const Solution<Command> *sol = nullptr);

void OutputAllStatistics(const std::vector<RollingSolution<Command>> &pop, std::ostream &os);

void OutputEvents(const RollingSolution<> &s, std::ostream &os);

void OutputSolution(const RollingSolution<> &s, std::ostream &os);

void CheckEvents(const RollingSolution<> &s, std::ostream &os);

void OutputUnit(const Unit *u, std::ostream &os);

// void CreateFileInCurrentFilePath(const std::string &filename, std::fstream &fs)
// {
//     std::string path(__FILE__);
//     std::size_t cut = path.rfind('/');
//     path = path.substr(0, cut) + '/' + filename;
//     fs.open(path, std::ios::out | std::ios::app);
// }

std::string CurrentFolder();

} // namespace sc2

#endif // DEBUG_USE_H
#endif // DEBUG