#include "Database.hpp"

#include "Table.hpp"
#include "Compiler.hpp"
#include "Entity.hpp"

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

    auto test = db.GetStringId("test");
    auto r1 = db.GetUnaryRelation(test);
    r1->Add(&e1);
    assert(r1->Count() == 1);

    auto r2 = db.GetBinaryRelation(test);
    Entity row1[2] = { e1, e1 };
    r2->Add(row1);
    assert(r2->Count() == 1);
    Entity row2[2] = { e1, e1 };
    r2->Add(row2);
    assert(r2->Count() == 1);

    x = db.GetStringId("x"), y = db.GetStringId("y");
    auto z = db.GetStringId("z");
    {
        Compilation c;
        bool bound;
        int slot = c.AddVariable(x, bound);
        assert(slot==0);
        assert(!bound);
        slot = c.AddVariable(y, bound);
        assert(slot == 1);
        assert(!bound);

        slot = c.AddVariable(x, bound);
        assert(slot == 0);
        assert(bound);

        slot = c.AddVariable(y, bound);
        assert(slot == 1);
        assert(bound);
    }

    {
        Compilation c;
        int slot;
        bool bound;
        slot = c.AddValue(Entity(EntityType::Integer, 1));

        // Let's create some branches
        c.AddVariable(x, bound);
        
        int branch1 = c.CreateBranch();
        slot = c.AddVariable(y, bound);
        assert(slot==2);
        assert(!bound);

        c.Branch(branch1);
        slot = c.AddVariable(z, bound);
        assert(slot==3);
        assert(!bound);

        // Slots in all branches are the same
        // But, the binding information is different.
        slot = c.AddVariable(y, bound);
        assert(slot==2);
        assert(!bound);
    }

    {
        Compilation c;
        bool bound;
        int branch = c.CreateBranch();

        int slot = c.AddVariable(x, bound);
        assert(!bound);
        slot = c.AddVariable(x, bound);
        assert(bound);
        
        c.Branch(branch);
        slot = c.AddVariable(x, bound);
        assert(!bound);
        slot = c.AddVariable(x, bound);
        assert(bound);
    }

    std::cout << "Tests passed\n";
}
