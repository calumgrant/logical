#include "Logical.hpp"
#include <cmath>

using namespace Logical;

void pi(Call & call)
{
    call.Set(0, M_PI);
    call.YieldResult();
}
