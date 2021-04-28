#include "Database.hpp"

#include "Table.hpp"

#undef _NDEBUG
#include <cassert>
#include <iostream>

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

    auto r1 = db.GetUnaryRelation("test");
    r1->Add(&e1);
    assert(r1->Count() == 1);

    auto r2 = db.GetBinaryRelation("test");
    Entity row1[2] = { e1, e1 };
    r2->Add(row1);
    assert(r2->Count() == 1);
    Entity row2[2] = { e1, e1 };
    r2->Add(row2);
    assert(r2->Count() == 1);

    std::cout << "Tests passed\n";
}
