#ifndef ROLLING_EA
#define ROLLING_EA

#include "differential_evolution.h"
#include "gnuplot-iostream.h"
#include "../simulator/simulator_pool.h"

class RollingEA
{
private:
    // methods
    Gnuplot gp_mo, gp_alg;

public:
    RollingEA(/* args */);
    ~RollingEA();
};

RollingEA::RollingEA(/* args */)
{
}

#endif //ROLLING_EA