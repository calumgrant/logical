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
        persist::map_file file { nullptr, 0, 0, 0, 16384, 16384, persist::temp_heap };
        StringTable st(file.data());

        int x = st.GetId(string_type("hello", file.data()));
        EQUALS(x, st.GetId(string_type("hello", file.data())));
        int y = st.GetId(string_type("hello2", file.data()));
        CHECK(x!=y);
        EQUALS(y, st.GetId(string_type("hello2", file.data())));
        EQUALS("hello", st.GetString(0));
        EQUALS("hello2", st.GetString(1));
    }
    
} stt;


class ProgramTests : public Test::Fixture<ProgramTests>
{
public:
    ProgramTests() : base("Program tests")
    {
        AddTest(&ProgramTests::TestBounds);
    }

    class DummyCall : public Logical::Call
    {
    };

    void TestBounds()
    {
        using namespace Logical;

        DummyCall call;
        Program<10> program1(call);
        
        Int testData[] = { 0, 0, 2, 4, 6, 6 };
        Int testData2[] = { 0,0, 2,0, 2,0, 2, 1};
                
        Logical::Internal::TableData tables[] = { { testData, 6 }, { testData2, 3 }};
        
        program1.tables = tables;
        
        // Lower/upper bound with 0 columns (all unbound = scan whole table)
        
        auto p = program1.lower_bound<1>(testData, 6);
        EQUALS(testData, p);
        p = program1.upper_bound<1>(testData, 6);
        EQUALS(testData + 6, p);
        
        // Lower/upper bound with 1 columns

        program1.sp[0] = -1;
        p = program1.lower_bound<1, 0>(testData, 6);
        EQUALS(0, p-testData);
        p = program1.upper_bound<1, 0>(testData, 6);
        EQUALS(0, p - testData);

        program1.sp[0] = 0;
        p = program1.lower_bound<1, 0>(testData, 6);
        EQUALS(0, p - testData);
        p = program1.upper_bound<1, 0>(testData, 6);
        EQUALS(2, p - testData);
        
        program1.sp[0] = 1;
        p = program1.lower_bound<1, 0>(testData, 6);
        EQUALS(2, p - testData);
        p = program1.upper_bound<1, 0>(testData, 6);
        EQUALS(2, p - testData);
        
        program1.sp[0] = 2;
        p = program1.lower_bound<1, 0>(testData, 6);
        EQUALS(2, p - testData);
        p = program1.upper_bound<1, 0>(testData, 6);
        EQUALS(3, p - testData);
        
        program1.sp[0] = 4;
        p = program1.lower_bound<1,0>(testData, 6);
        EQUALS(3, p - testData);
        p = program1.upper_bound<1,0>(testData, 6);
        EQUALS(4, p - testData);

        program1.sp[0] = 6;
        p = program1.lower_bound<1,0>(testData, 6);
        EQUALS(4, p - testData);
        p = program1.upper_bound<1,0>(testData, 6);
        EQUALS(6, p - testData);

        program1.sp[0] = 7;
        p = program1.lower_bound<1,0>(testData, 6);
        EQUALS(6, p - testData);
        p = program1.upper_bound<1,0>(testData, 6);
        EQUALS(6, p - testData);

        // arity=2, columns = 0
        p = program1.lower_bound<2>(testData2, 3);
        EQUALS(0, p - testData2);
        p = program1.upper_bound<2>(testData2, 3);
        EQUALS(6, p - testData2);
        
        // arity=2, columns=1
        
        program1.sp[3] = 0;
        p = program1.lower_bound<2,3>(testData2, 3);
        EQUALS(0, p-testData2);
        p = program1.upper_bound<2,3>(testData2, 3);
        EQUALS(2, p-testData2);
        
        program1.sp[3] = 1;
        p = program1.lower_bound<2,3>(testData2, 3);
        EQUALS(2, p-testData2);
        p = program1.upper_bound<2,3>(testData2, 3);
        EQUALS(2, p-testData2);

        program1.sp[3] = 2;
        p = program1.lower_bound<2,3>(testData2, 3);
        EQUALS(2, p-testData2);
        p = program1.upper_bound<2,3>(testData2, 3);
        EQUALS(6, p-testData2);
        
        // arity=2, columns=2
        program1.sp[3] = 0;
        program1.sp[2] = 0;
        p = program1.lower_bound<2,3,2>(testData2, 3);
        EQUALS(0, p-testData2);
        p = program1.upper_bound<2,3,2>(testData2, 3);
        EQUALS(2, p-testData2);

        program1.sp[3] = 1;
        program1.sp[2] = 1;
        p = program1.lower_bound<2,3>(testData2, 3);
        EQUALS(2, p-testData2);
        p = program1.upper_bound<2,3>(testData2, 3);
        EQUALS(2, p-testData2);

        program1.sp[3] = 2;
        program1.sp[2] = 0;
        p = program1.lower_bound<2,3>(testData2, 3);
        EQUALS(2, p-testData2);
        p = program1.upper_bound<2,3>(testData2, 3);
        EQUALS(6, p-testData2);
    }
} pt;

int main()
{
    OptimizerImpl opt;
    DatabaseImpl db(opt, nullptr, 1000);
    auto e1 = db.CreateInt(0);

    auto test = db.GetStringId("test");
    auto & r1 = db.GetUnaryRelation(test);
    r1.Add(&e1);
    assert(r1.GetCount() == 1);

    auto & r2 = db.GetBinaryRelation(test);
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
