#include "Database.hpp"

#include "Compiler.hpp"
#include "Entity.hpp"
#include "CompoundName.hpp"
#include "Binary.hpp"
#include "DatabaseImpl.hpp"
#include "OptimizerImpl.hpp"

#include "Program.hpp"

#include <simpletest.hpp>

#undef _NDEBUG
#undef NDEBUG
#include <cassert>
#include <iostream>
#include <sstream>

void TestSerialization(const Entity &e)
{
    char buffer[20] = {0};
    char * p = buffer;
    WriteBinaryEntity(e, p, p+20);
    p = buffer;
    auto e4 = ReadBinaryEntity(p, p+20);
 
    if(e != e4)
    {
        p = buffer;
        WriteBinaryEntity(e, p, p+20);
        p = buffer;
        e4 = ReadBinaryEntity(p, p+20);
    }
    
    std::stringstream ss;
    std::ostream_iterator<char> it{ss};
    WriteBinaryEntity(e, it);
    auto str = ss.str();
    auto len = str.size();
    std::istreambuf_iterator<char> ii{ss};
    auto e2 = ReadBinaryEntity(ii);
    if(!(e == e2))
    {
        std::stringstream ss;
        std::ostream_iterator<char> it{ss};
        WriteBinaryEntity(e, it);
        std::istreambuf_iterator<char> ii{ss};
        auto e3 = ReadBinaryEntity(ii);
    }
    assert(e == e2);
}

class DemoTest : public Test::Fixture<DemoTest>
{
public:
    // The default constructor adds tests
    DemoTest()
    {
        name = "This is a demo";

        AddTest(&DemoTest::f);
        AddTest(&DemoTest::g);
        AddTest(&DemoTest::g);
    }

    int count = 10;

    void f()
    {
        EQUALS(1,1);
    }

    void g()
    {
        EQUALS(10, count);
        ++count;
    }
} t2;

class Test2 : public Test::Fixture<Test2>
{
public:
    Test2() : base("Test2")
    {
    }
} t3;


class StringTableTests : public Test::Fixture<StringTableTests>
{
public:
    StringTableTests() : base("String table")
    {
        AddTest(&StringTableTests::Test);
    }

    void Test()
    {
#if MMAP_ALLOCATOR
        persist::map_file file { nullptr, 0, 0, 0, 16384, 16384, persist::temp_heap };
        
        auto & storage = file.data();
        StringTable st(storage);
#else
        MemoryCounter storage(16384);
        StringTable st(storage);
#endif

        int x = st.GetId("hello");
        EQUALS(x, st.GetId("hello"));
        int y = st.GetId("hello2");
        CHECK(x!=y);
        EQUALS(y, st.GetId("hello2"));
        EQUALS(std::string("hello"), st.GetString(0));
        EQUALS(std::string("hello2"), st.GetString(1));
    }
    
} stt;


int main(int argc, char**argv)
{
    OptimizerImpl opt;
    DatabaseImpl db(argv[0], opt, nullptr, 1000);
    auto e1 = db.CreateInt(0);

    PredicateName test(1, db.GetStringId("test"));
    auto & r1 = db.GetRelation(test);
    r1.Add(&e1);
    assert(r1.GetCount() == 1);

    auto & r2 = db.GetRelation(test);
    Entity row1[2] = { e1, e1 };
    r2.Add(row1);
    assert(r2.GetCount() == 1);
    Entity row2[2] = { e1, e1 };
    r2.Add(row2);
    assert(r2.GetCount() == 1);

    StringId x = db.GetStringId("x"), y = db.GetStringId("y");
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
    
    {
        std::vector<int> l1 { 1, 2, 3 }, l2 {3, 2, 1, 3}, l3{1}, l4{1,3}, l5{}, l6{2,3,4};
        CompoundName name1(l1), name2(l2), name3(l3), name4(l4), name5(l5), name6(l6);
        assert(name1 == name2);
        assert(name1 <= name2);
        assert(name3 <= l1);
        assert(name5 <= name5);
        assert(name1 <= name1);
        assert(name5 <= name3);
        assert(name5 <= name1);
        assert(name3 <= name4);
        assert(name1 <= name2);
        assert(!(name1 <= name6));
        assert(!(name6 <= name1));
    }
    
    {
        for(int i=-1000; i<1000; ++i)
            TestSerialization(Entity{EntityType::Integer, i});
        TestSerialization(Entity{EntityType::Integer, 16324});
        TestSerialization(Entity{EntityType::Integer, 0xffff});
        TestSerialization(Entity{EntityType::Integer, 0x1ffff});
        TestSerialization(Entity{EntityType::Integer, 0x7fffffff});
        TestSerialization(Entity{EntityType::Integer, (int)0xafffffff});
        
        TestSerialization(Entity{EntityType::Float, 3.14f});
        TestSerialization(Entity{EntityType::Float, -1.0f});

        TestSerialization(Entity{EntityType::Boolean, true});
        TestSerialization(Entity{EntityType::Boolean, false});
        
        TestSerialization(Entity{EntityType::String, 0});
        TestSerialization(Entity{EntityType::String, 0x7f00});
        TestSerialization(Entity{EntityType::String, 0x8000});
        TestSerialization(Entity{EntityType::String, 0xffff});
        TestSerialization(Entity{EntityType::String, 0x10000});
        TestSerialization(Entity{EntityType::String, -1});

        TestSerialization(Entity{EntityType::AtString, 0});
        TestSerialization(Entity{EntityType::AtString, 0x7f00});
        TestSerialization(Entity{EntityType::AtString, 0x8000});
        TestSerialization(Entity{EntityType::AtString, 0xffff});
        TestSerialization(Entity{EntityType::AtString, 0x10000});
        TestSerialization(Entity{EntityType::AtString, -1});
    }
}
