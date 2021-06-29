
#include "../../include/HashTable.hpp"
#include "../../external/simpletest/simpletest.hpp"

using namespace Logical;

class TablesTest : public Test::Fixture<TablesTest>
{
public:
    TablesTest() : base("Tables")
    {
        AddTest(&TablesTest::SimpleUnaryTableTest);
    }
    
    void SimpleUnaryTableTest()
    {
        SimpleTable<FixedArity<1>> t1;
        
        EQUALS(0, t1.size());
        
        t1.Add(0);
        t1.Add(3);
        t1.Add(2);
        t1.Add(0);
        EQUALS(4, t1.size());
                
        SortedTable<Logical::FixedArity<1>> t2(std::move(t1));
        EQUALS(3, t2.size());
        
        auto e = t2.Find();
        Int v;
        CHECK(e.Next(v));
        EQUALS(0, v);
        CHECK(e.Next(v));
        EQUALS(2, v);
        CHECK(e.Next(v));
        EQUALS(3, v);
        CHECK(!e.Next(v));
        
        SimpleTable<Logical::VariableArity> t3(1);
        EQUALS(0, t3.size());
        
        t3.Add(0);
        t3.Add(3);
        t3.Add(2);
        t3.Add(0);
        EQUALS(4, t3.size());
        SortedTable<Logical::VariableArity> t4(std::move(t3));
        EQUALS(3, t4.size());

        auto e2 = t4.Find();
        CHECK(e2.Next(v));
        EQUALS(0, v);
        CHECK(e2.Next(v));
        EQUALS(2, v);
        CHECK(e2.Next(v));
        EQUALS(3, v);
        CHECK(!e2.Next(v));
    }
} tt;
