
#include "../../include/HashTable.hpp"
#include "../../external/simpletest/simpletest.hpp"

using namespace Logical;

class TablesTest : public Test::Fixture<TablesTest>
{
public:
    TablesTest() : base("Tables")
    {
        AddTest(&TablesTest::SimpleUnaryTableTest);
        AddTest(&TablesTest::TestHash);
        AddTest(&TablesTest::HashTable1);
    }
    
    void SimpleUnaryTableTest()
    {
        SimpleTable<StaticArity<1>> t1;
        
        EQUALS(0, t1.size());
        
        t1.Add(0);
        t1.Add(3);
        t1.Add(2);
        t1.Add(0);
        EQUALS(4, t1.size());
                
        SortedTable<StaticArity<1>> t2(std::move(t1));
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
        
        SimpleTable<DynamicArity> t3(1);
        EQUALS(0, t3.size());
        
        t3.Add(0);
        t3.Add(3);
        t3.Add(2);
        t3.Add(0);
        EQUALS(4, t3.size());
        SortedTable<DynamicArity> t4(std::move(t3));
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
    
    void TestHash()
    {
        EQUALS(0, (Internal::BoundHash<false,false>(1,2)));
        EQUALS(1, (Internal::BoundHash<true, false>(1,2)));
        EQUALS(2, (Internal::BoundHash<false,true>(1,2)));
        EQUALS(2*317 + 1, (Internal::BoundHash<true,true>(1,2)));

        EQUALS(0, (Internal::Hash(StaticBinding<false,false>(), 1,2)));
        EQUALS(1, (Internal::Hash(StaticBinding<true, false>(), 1,2)));
        EQUALS(2, (Internal::Hash(StaticBinding<false,true>(), 1,2)));
        EQUALS(2*317 + 1, (Internal::Hash(StaticBinding<true,true>(), 1,2)));

        EQUALS(0, Internal::Hash(DynamicBinding(false,false), 1, 2));
        EQUALS(1, Internal::Hash(DynamicBinding(true,false), 1, 2));
        EQUALS(2, Internal::Hash(DynamicBinding(false,true), 1, 2));
        EQUALS(2*317+1, Internal::Hash(DynamicBinding(true,true), 1, 2));
        
        Int row[] = { 1, 2 };
        EQUALS(0, (Internal::Hash(StaticBinding<false,false>(), row)));
        EQUALS(1, (Internal::Hash(StaticBinding<true, false>(), row)));
        EQUALS(2, (Internal::Hash(StaticBinding<false,true>(), row)));
        EQUALS(2*317 + 1, (Internal::Hash(StaticBinding<true,true>(), row)));

        EQUALS(0, Internal::Hash(DynamicBinding(false,false), row));
        EQUALS(1, Internal::Hash(DynamicBinding(true,false), row));
        EQUALS(2, Internal::Hash(DynamicBinding(false,true), row));
        EQUALS(2*317+1, Internal::Hash(DynamicBinding(true,true), row));
        
        EQUALS(2*317+1, Internal::Hash(DynamicArity(2), row));
        EQUALS(2*317+1, Internal::Hash(StaticArity<2>(), row));

        EQUALS(0, Internal::Hash(DynamicArity(0), row));
        EQUALS(0, Internal::Hash(StaticArity<0>(), row));

        EQUALS(0, Internal::Hash(DynamicArity(0)));
        EQUALS(0, Internal::Hash(StaticArity<0>()));
    }
    
    void HashTable1()
    {
        HashTable<StaticArity<1>> t1;
        
        t1.Find(StaticBinding<true,false,true>(), 1,2,3);
    }
} tt;
