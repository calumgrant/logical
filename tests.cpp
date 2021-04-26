#include "Database.hpp"
#undef _NDEBUG
#include <cassert>

int main()
{
    StringTable s1;
    int x = s1.GetId("hello");
    assert(s1.GetId("hello")==x);
    int y = s1.GetId("hello2");
    assert(x!=y);
    assert(s1.GetId("hello2")==y);

    assert(s1.GetString(0) == "hello");
    assert(s1.GetString(1) == "hello2");
}