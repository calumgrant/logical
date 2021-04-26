#include "Database.hpp"
#undef _NDEBUG
#include <cassert>

int main()
{
    StringTable st;
    int x = st.GetId("hello");
    assert(st.GetId("hello")==x);
    int y = st.GetId("hello2");
    assert(x!=y);
    assert(st.GetId("hello2")==y);

    assert(st.GetString(0) == "hello");
    assert(st.GetString(1) == "hello2");

    Database db;
    auto e1 = db.CreateInt(0);

    auto &r = db.GetRelation("test", 1);
    r.Add(e1);
}
