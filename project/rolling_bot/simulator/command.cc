#include "command.h"

namespace sc2
{
std::map<Tag, std::deque<ActionRaw>> Command::ConmmandsVecToDeque(const std::vector<Command> &commands) // 使用deque，方便进行取放操作
{
    std::map<Tag, std::deque<ActionRaw>> command_map;
    for (const Command &c : commands)
    {
        command_map[c.unit_tag] = std::deque<ActionRaw>(); // 保证序列为空
        std::deque<ActionRaw> &current_deque = command_map[c.unit_tag];
        std::transform(c.actions.cbegin(), c.actions.cend(), std::back_inserter(current_deque), [](const ActionRaw &action) { return action; }); // 转换
    }
    return command_map;
}

} // namespace sc2
