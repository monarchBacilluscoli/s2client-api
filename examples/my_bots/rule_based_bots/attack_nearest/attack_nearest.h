#ifndef ATTACK_NEAREST_H
#define ATTACK_NEAREST_H

#include<iostream>
#include<sc2api/sc2_api.h>
#include"../rule_based_bot.h"

namespace sc2 {
    // contains some basic methods
    class attack_nearest :public rule_based_bot {
    public:
        attack_nearest() = default;
        ~attack_nearest() = default;

        virtual void OnUnitIdle(const Unit* unit) final;
        virtual void OnGameStart() final;
        virtual void OnStep() final;

    private:
        
    };
}

#endif // !ATTACK_NEAREST_H
