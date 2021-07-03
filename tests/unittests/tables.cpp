
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
        AddTest(&TablesTest::UnaryTableTests);
    }

    template<typename Table>
    void AddTableData(Table & t)
    {
        EQUALS(0, t.size());
        
        Int testData[] = { 0, 3, 2, 0 };
        
        for(auto i : testData)
        {
            t.Add(i);
        }
        EQUALS(4, t.size());
    }

    template<typename Unbound, typename Bound, typename Index>
    void TestIndexBinding(Unbound u, Bound b, Index i)
    {
        // Scan all data
        Enumerator e;
        Int v;
        Int sorted[] = { 0, 2, 3 };

        i.Find(e, u);
        for(int j=0; j<3; ++j)
        {
            CHECK(i.Next(e, u, v));
            EQUALS(sorted[j], v);
        }
        CHECK(!i.Next(e, u, v));
        
        Int k = -1;
        i.Find(e, b, k);
        CHECK(!i.Next(e, u, k));
        for(int j=0; j<3; ++j)
        {
            k = sorted[j];
            i.Find(e, b, k);
            CHECK(i.Next(e, u, k));
            CHECK(!i.Next(e, u, k));
        }

        i.Find(e, u);
        for(int j=0; j<3; ++j)
        {
            CHECK(i.Next(e, u, &v));
            EQUALS(sorted[j], v);
        }
        CHECK(!i.Next(e, u, &v));
        
        k = -1;
        i.Find(e, b, &k);
        CHECK(!i.Next(e, u, &k));
        for(int j=0; j<3; ++j)
        {
            k = sorted[j];
            i.Find(e, b, k);
            CHECK(i.Next(e, u, &k));
            CHECK(!i.Next(e, u, &k));
        }
    }
    
    template<typename Index>
    void TestIndex(Index t)
    {
        TestIndexBinding(StaticBinding<false>(), StaticBinding<true>(), t);
        TestIndexBinding(DynamicBinding(false), DynamicBinding(true), t);
    }
    
    void UnaryTableTests()
    {
        BasicTable<StaticArity<1>> t1;
        AddTableData(t1);

        SortedTable<StaticArity<1>> t2(std::move(t1));
        TestIndex(t2.GetIndex());

        BasicTable<DynamicArity> t3(1);
        AddTableData(t3);

        SortedTable<DynamicArity> t4(std::move(t3));
        TestIndex(t4.GetIndex());
    }
    
    void SimpleUnaryTableTest()
    {
        BasicTable<StaticArity<1>> t1;
        
        EQUALS(0, t1.size());
        
        t1.Add(0);
        t1.Add(3);
        t1.Add(2);
        t1.Add(0);
        EQUALS(4, t1.size());
                
        SortedTable<StaticArity<1>> t2(std::move(t1));
        EQUALS(3, t2.size());
        
        auto i1 = t2.GetIndex();
        Enumerator e;
        StaticBinding<false> b;
        i1.Find(e, b);
        Int v;
        CHECK(i1.Next(e, b, v));
        EQUALS(0, v);
        CHECK(i1.Next(e,b,v));
        EQUALS(2, v);
        CHECK(i1.Next(e,b,v));
        EQUALS(3, v);
        CHECK(!i1.Next(e,b,v));
        
        BasicTable<DynamicArity> t3(1);
        EQUALS(0, t3.size());
        
        t3.Add(0);
        t3.Add(3);
        t3.Add(2);
        t3.Add(0);
        EQUALS(4, t3.size());
        SortedTable<DynamicArity> t4(std::move(t3));
        EQUALS(3, t4.size());

        auto i4 = t4.GetIndex();
        i4.Find(e, b);
        CHECK(i4.Next(e, b, v));
        EQUALS(0, v);
        CHECK(i4.Next(e, b, v));
        EQUALS(2, v);
        CHECK(i4.Next(e, b, v));
        EQUALS(3, v);
        CHECK(!i4.Next(e, b, v));
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
