#include "Logical.hpp"
#include <cmath>
#include <random>

using namespace Logical;

void pi(Call & call)
{
    call.Set(0, M_PI);
    call.YieldResult();
}

void rand(Call & call)
{
    Int index;
    if(call.Get(0, index))
    {
        std::mt19937 rng;
        rng.seed(0);
        rng.discard(index);
        call.Set(1, (Int)rng());
        call.YieldResult();
    }
}
