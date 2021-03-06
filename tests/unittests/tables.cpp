
#include "../../include/HashTable.hpp"
#include "../../external/simpletest/simpletest.hpp"

using namespace Logical;

class TablesTest : public Test::Fixture<TablesTest>
{
public:
    TablesTest() : base("Tables")
    {
        AddTest(&TablesTest::SimpleUnaryTableTest);
        // AddTest(&TablesTest::TestHash);
        AddTest(&TablesTest::HashTable1);
        AddTest(&TablesTest::UnaryTableTests);
        AddTest(&TablesTest::BinaryTableTests);
        AddTest(&TablesTest::NaryTableTests);
        AddTest(&TablesTest::HashTable2);
        AddTest(&TablesTest::HashTable3);
        AddTest(&TablesTest::HashTable4);
        AddTest(&TablesTest::Deltas);
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
        CHECK(!i.Next(e, b, k));
        for(int j=0; j<3; ++j)
        {
            k = sorted[j];
            i.Find(e, b, k);
            CHECK(i.Next(e, b, k));
            CHECK(!i.Next(e, b, k));
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

        BasicTable<DynamicArity> t3(DynamicArity(1));
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
        
        BasicTable<DynamicArity> t3(DynamicArity(1));
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
        EQUALS(0, (Internal::BoundHashI<false,false>(1,2)));
        EQUALS(1, (Internal::BoundHashI<true, false>(1,2)));
        EQUALS(2, (Internal::BoundHashI<false,true>(1,2)));
        EQUALS(2*317 + 1, (Internal::BoundHashI<true,true>(1,2)));

        EQUALS(0, (Internal::BoundHash(StaticBinding<false,false>(), 1,2)));
        EQUALS(1, (Internal::BoundHash(StaticBinding<true, false>(), 1,2)));
        EQUALS(2, (Internal::BoundHash(StaticBinding<false,true>(), 1,2)));
        EQUALS(2*317 + 1, (Internal::BoundHash(StaticBinding<true,true>(), 1,2)));

        EQUALS(0, Internal::BoundHash(DynamicBinding(false,false), 1, 2));
        EQUALS(1, Internal::BoundHash(DynamicBinding(true,false), 1, 2));
        EQUALS(2, Internal::BoundHash(DynamicBinding(false,true), 1, 2));
        EQUALS(2*317+1, Internal::BoundHash(DynamicBinding(true,true), 1, 2));
        
        Int row[] = { 1, 2 };
        EQUALS(0, (Internal::BoundHash(StaticBinding<false,false>(), row)));
        EQUALS(1, (Internal::BoundHash(StaticBinding<true, false>(), row)));
        EQUALS(2, (Internal::BoundHash(StaticBinding<false,true>(), row)));
        EQUALS(2*317 + 1, (Internal::BoundHash(StaticBinding<true,true>(), row)));

        EQUALS(0, Internal::BoundHash(DynamicBinding(false,false), row));
        EQUALS(1, Internal::BoundHash(DynamicBinding(true,false), row));
        EQUALS(2, Internal::BoundHash(DynamicBinding(false,true), row));
        EQUALS(2*317+1, Internal::BoundHash(DynamicBinding(true,true), row));
        
        EQUALS(2*317+1, Internal::Hash(DynamicArity(2), row));
        EQUALS(2*317+1, Internal::Hash(StaticArity<2>(), row));

        EQUALS(0, Internal::Hash(DynamicArity(0), row));
        EQUALS(0, Internal::Hash(StaticArity<0>(), row));

        EQUALS(0, Internal::Hash(DynamicArity(0)));
        EQUALS(0, Internal::Hash(StaticArity<0>()));
    }
    
    template<typename T, typename Arity>
    void TestUnaryHashTable(Arity a)
    {
        bool added;
        Int x;
        Enumerator e;
        T t1(a);

        EQUALS(0, t1.size());
        added = false;
        t1.Add(added, 1);
        x=2;
        t1.Add(added, &x);
        t1.Add(added, 3);
        CHECK(added);
        EQUALS(3, t1.size());
        added = false;
        t1.Add(added, 2);
        CHECK(!added);
        EQUALS(3, t1.size());
        
        auto b = Internal::GetBoundBinding(a);
        auto f = Internal::GetUnboundBinding(a);
        
        auto i1 = t1.GetScanIndex();
        auto i2 = t1.GetProbeIndex();

        i1.Find(e, f);
        CHECK(i1.Next(e,f,x));
        CHECK(i1.Next(e,f,x));
        CHECK(i1.Next(e,f,x));
        CHECK(!i1.Next(e,f,x));
        
        i1.Find(e, f);
        CHECK(i1.Next(e,f,&x));
        CHECK(i1.Next(e,f,&x));
        CHECK(i1.Next(e,f,&x));
        CHECK(!i1.Next(e,f,&x));

        x = 2;
        i2.Find(e, b, x);
        CHECK(i2.Next(e,b,x));
        CHECK(!i2.Next(e,b,x));

        i2.Find(e, b, &x);
        CHECK(i2.Next(e,b,&x));
        
        x=10;
        i2.Find(e, b, x);
        CHECK(!i2.Next(e,b,x));

        i2.Find(e, b, &x);
        CHECK(!i2.Next(e,b,&x));
        
        for(Int i=0; i<1000; ++i)
            t1.Add(added, i);

        for(Int i=0; i<1000; ++i)
            t1.Add(added, i);

        i1 = t1.GetScanIndex();
        i2 = t1.GetProbeIndex();
        EQUALS(1000, i1.rows());
        
        for(Int i=0; i<1000; ++i)
        {
            i2.Find(e, b, i);
            CHECK(i2.Next(e, b, i));
            CHECK(!i2.Next(e, b, i));
        }
    }
    
    void HashTable1()
    {
        TestUnaryHashTable<BasicHashTable<StaticArity<1>>>(StaticArity<1>());
        TestUnaryHashTable<BasicHashTable<DynamicArity>>(DynamicArity(1));

        TestUnaryHashTable<OpenHashTable<StaticArity<1>>>(StaticArity<1>());
        TestUnaryHashTable<OpenHashTable<DynamicArity>>(DynamicArity(1));

        // Unary hash table tests
        
        BasicHashTable<StaticArity<1>> t1;
        bool added = false;
        Enumerator e;
        StaticBinding<false> f;
        StaticBinding<true> b;
        Int x;
        
        EQUALS(0, t1.size());
        t1.Add(added, 1);
        t1.Add(added, 2);
        t1.Add(added, 3);
        CHECK(added);
        EQUALS(3, t1.size());
        added = false;
        t1.Add(added, 2);
        CHECK(!added);
        EQUALS(3, t1.size());
        
        // Try a scan
        auto i1 = t1.GetScanIndex();
        i1.Find(e, f);
        CHECK(i1.Next(e,f,x));
        CHECK(i1.Next(e,f,x));
        CHECK(i1.Next(e,f,x));
        CHECK(!i1.Next(e,f,x));
        
        i1.Find(e, f);
        CHECK(i1.Next(e,f,&x));
        CHECK(i1.Next(e,f,&x));
        CHECK(i1.Next(e,f,&x));
        CHECK(!i1.Next(e,f,&x));
        
        // Try a probe
        auto i2 = t1.GetProbeIndex();
        x = 2;
        i2.Find(e, b, x);
        CHECK(i2.Next(e, b, x));
        CHECK(!i2.Next(e, b, x));
        
        x = 3;
        i2.Find(e, b, x);
        CHECK(i2.Next(e, b, x));
        CHECK(!i2.Next(e, b, x));

        x = 4;
        i2.Find(e, b, x);
        CHECK(!i2.Next(e, b, x));
    }
    
    template<typename Table, typename Arity>
    void TestNaryHash(Arity a)
    {
        Table t1(a);
        bool added = false;
        Enumerator e;
        
        t1.Add(added, 1, 2, 3);
        t1.Add(added, 1, 2, 4);
        t1.Add(added, 1, 3, 5);
        CHECK(added);

        Int row[3] = { 1, 3, 5 };
        added = false;
        t1.Add(added, row);
        CHECK(!added);
        
        auto i1 = t1.GetProbeIndex();
        auto bbb = Internal::GetBoundBinding(a);
        i1.Find(e, bbb, 1, 2, 3);
        Int x=1,y=2,z=3;
        CHECK(i1.Next(e, bbb, x, y, z));
        CHECK(!i1.Next(e, bbb, x, y, z));

        x=4;
        i1.Find(e, bbb, x,y,z);
        CHECK(!i1.Next(e, bbb, x, y, z));

        i1.Find(e, bbb, row);
        CHECK(i1.Next(e, bbb, row));
        CHECK(!i1.Next(e, bbb, x, y, z));

        row[0] = 10;
        i1.Find(e, bbb, row);
        CHECK(!i1.Next(e, bbb, row));
    }
    
    void HashTable2()
    {
        TestNaryHash<OpenHashTable<StaticArity<3>>>(StaticArity<3>());
        TestNaryHash<OpenHashTable<DynamicArity>>(DynamicArity(3));

        TestNaryHash<BasicHashTable<StaticArity<3>>>(StaticArity<3>());
        TestNaryHash<BasicHashTable<DynamicArity>>(DynamicArity(3));
    }

    template<typename Table>
    void MakeBinary(Table &t)
    {
        EQUALS(0, t.size());
        Int data[] = { 1, 1, 1, 0, 2, 2, 2, 3, 2, 2 };
        
        for(int i=0; i<10; i+=2)
        {
            t.Add(data[i], data[i+1]);
            t.Add(data+i);
        }
        EQUALS(10, t.size());
    }
    
    template<typename FF, typename BF, typename BB, typename Index>
    void TestBinary(FF ff, BF bf, BB bb, const Index & i)
    {
        Enumerator e;
        Int x, y;
        Int row[2];
        
        // Find all (scan)
        i.Find(e, ff);
        CHECK(i.Next(e, ff, x, y));
        EQUALS(1, x);
        EQUALS(0, y);
        CHECK(i.Next(e, ff, x, y));
        EQUALS(1, x);
        EQUALS(1, y);
        CHECK(i.Next(e, ff, x, y));
        EQUALS(2, x);
        EQUALS(2, y);
        CHECK(i.Next(e, ff, x, y));
        EQUALS(2, x);
        EQUALS(3, y);
        CHECK(!i.Next(e, ff, x, y));

        i.Find(e, ff);
        CHECK(i.Next(e, ff, row));
        EQUALS(1, row[0]);
        EQUALS(0, row[1]);
        CHECK(i.Next(e, ff, row));
        EQUALS(1, row[0]);
        EQUALS(1, row[1]);
        CHECK(i.Next(e, ff, row));
        EQUALS(2, row[0]);
        EQUALS(2, row[1]);
        CHECK(i.Next(e, ff, row));
        EQUALS(2, row[0]);
        EQUALS(3, row[1]);
        CHECK(!i.Next(e, ff, row));

        // Find one column
        x = 2;
        y = -1;
        i.Find(e, bf, x);
        CHECK(i.Next(e, bf, x, y));
        EQUALS(2, y);
        CHECK(i.Next(e, bf, x, y));
        EQUALS(3, y);
        CHECK(!i.Next(e, bf, x, y));

        x = 10;
        i.Find(e, bf, x);
        CHECK(!i.Next(e, bf, x, y));
        
        row[0] = 2;
        row[1] = -1;
        i.Find(e, bf, row);
        CHECK(i.Next(e, bf, row));
        EQUALS(2, row[1]);
        CHECK(i.Next(e, bf, row));
        EQUALS(3, row[1]);
        CHECK(!i.Next(e, bf, row));
        
        row[0] = 10;
        i.Find(e, bf, row);
        CHECK(!i.Next(e, bf, row));
        
        // Find two columns (probe)
        x=2, y=2;
        i.Find(e, bb, x, y);
        CHECK(i.Next(e, bb));
        CHECK(!i.Next(e, bb));
        
        y = 100;
        i.Find(e, bb, x, y);
        CHECK(!i.Next(e, bb));
        
        row[0] = 100;
        i.Find(e, bb, row);
        CHECK(!i.Next(e, bb));
    }
    
    void BinaryTableTests()
    {
        BasicTable<StaticArity<2>> t1;
        BasicTable<DynamicArity> t2(DynamicArity(2));
        
        MakeBinary(t1);
        SortedTable<StaticArity<2>> t3(std::move(t1));
        EQUALS(4, t3.size());
        TestBinary(StaticBinding<false,false>(), StaticBinding<true,false>(), StaticBinding<true,true>(), t3.GetIndex());
        
        MakeBinary(t2);
        SortedTable<DynamicArity> t4(std::move(t2));
        EQUALS(4, t4.size());
        TestBinary(DynamicBinding(false,false), DynamicBinding(true,false), DynamicBinding(true,true), t4.GetIndex());
    }
    
    void NaryTableTests()
    {
        BasicTable<StaticArity<4>> t1;
        t1.Add(1,2,3,4);
        t1.Add(1,2,3,3);
        t1.Add(1,2,3,3);
        SortedTable<StaticArity<4>> t2(std::move(t1));
        
        Int a,b,c,d;
        Int row[4];
        StaticBinding<true,true,false,false> bbff;
        Enumerator e;
        auto index = t2.GetIndex();
        
        a=1;
        b=2;
        index.Find(e, bbff, a, b);
        CHECK(index.Next(e, bbff, a, b, c, d));
        EQUALS(3, c);
        EQUALS(3, d);
        CHECK(index.Next(e, bbff, a, b, c, d));
        EQUALS(3, c);
        EQUALS(4, d);
    }

    template<typename Table, typename Arity, typename Binding>
    void TestIndexes(Arity a, Binding fbf)
    {
        Table t(a);
        auto i1 = t.MakeIndex(fbf);
        bool added;
        Enumerator e;
        Int x,y,z;

        t.Add(added, 1, 2, 3);
        auto i2 = i1.GetIndex();
        
        y = 2;
        i2.Find(e, fbf, x, y, z);
        CHECK(i2.Next(e, fbf, x,y,z));
        CHECK(!i2.Next(e, fbf, x,y,z));
        
        for(x=0; x<10; ++x)
            for(y=0; y<10; ++y)
                for(z=0; z<10; ++z)
                    t.Add(added, x,y,z);
        
        EQUALS(1000, t.size());

        i2 = i1.GetIndex();
        for(y=0; y<10; ++y)
        {
            int count=0;
            i2.Find(e, fbf, x, y, z);
            while(i2.Next(e, fbf, x, y, z))
            {
                ++count;
            }
            if(count != 100)
            {
                i2.Find(e, fbf, x, y, z);
                while(i2.Next(e, fbf, x, y, z))
                {
                    std::cout << "(" << x << "," << y << "," << z << ")\n";
                }
            }
            EQUALS(100, count);
        }
        
        t=Table(a);
        auto i3 = t.MakeIndex(fbf);
        Int row[3];

        row[0] = 1;
        row[1] = 2;
        row[2] = 3;
        t.Add(added, row);
        i2 = i3.GetIndex();

        row[1] = 2;
        i2.Find(e, fbf, row);
        CHECK(i2.Next(e, fbf, row));
        CHECK(!i2.Next(e, fbf, row));
        
        for(x=0; x<10; ++x)
            for(y=0; y<10; ++y)
                for(z=0; z<10; ++z)
                {
                    row[0] = x;
                    row[1] = y;
                    row[2] = z;
                    t.Add(added, row);
                }
        
        EQUALS(1000, t.size());

        i2 = i1.GetIndex();
        for(y=0; y<10; ++y)
        {
            int count=0;
            row[1] = y;
            i2.Find(e, fbf, row);
            while(i2.Next(e, fbf, row))
            {
                ++count;
            }
            if(count != 100)
                EQUALS(100, count);
        }
    }
    
    template<typename Table, typename Arity, typename Binding>
    void TestIndexes2(Arity a, Binding fbb)
    {
        // Test
        Table t(a);
        auto i1 = t.MakeIndex(fbb);
        bool added;
        Enumerator e;
        Int x,y,z;
        Int row[3];
        
        for(x=0; x<10; ++x)
            for(y=0; y<10; ++y)
                for(z=0; z<10; ++z)
                    t.Add(added, x,y,z);
        
        EQUALS(1000, t.size());

        auto i2 = i1.GetIndex();
        for(y=0; y<10; ++y)
        {
            for(z=0; z<10; ++z)
            {
                int count=0;
                for(i2.Find(e, fbb, x, y, z); i2.Next(e, fbb, x, y, z); count++)
                    ;
                EQUALS(10, count);

                count=0;
                row[1] = y;
                row[2] = z;
                for(i2.Find(e, fbb, row); i2.Next(e, fbb, row); count++)
                    ;
                EQUALS(10, count);

            }
        }
    }
    
    void HashTable3()
    {
        TestIndexes<BasicHashTable<StaticArity<3>>>(StaticArity<3>(), StaticBinding<false,true,false>());
        TestIndexes<BasicHashTable<DynamicArity>>(DynamicArity(3), DynamicBinding(false,true,false));
        TestIndexes<BasicHashTable<StaticArity<3>>>(StaticArity<3>(), DynamicBinding(false,true,false));
        TestIndexes<BasicHashTable<DynamicArity>>(DynamicArity(3), StaticBinding<false,true,false>());

        TestIndexes<OpenHashTable<StaticArity<3>>>(StaticArity<3>(), StaticBinding<false,true,false>());
        TestIndexes<OpenHashTable<DynamicArity>>(DynamicArity(3), DynamicBinding(false,true,false));
        TestIndexes<OpenHashTable<StaticArity<3>>>(StaticArity<3>(), DynamicBinding(false,true,false));
        TestIndexes<OpenHashTable<DynamicArity>>(DynamicArity(3), StaticBinding<false,true,false>());
    }

    void HashTable4()
    {
        TestIndexes2<BasicHashTable<StaticArity<3>>>(StaticArity<3>(), StaticBinding<false, true, true>());
        TestIndexes2<BasicHashTable<DynamicArity>>(DynamicArity(3), DynamicBinding(false, true, true));
        TestIndexes2<BasicHashTable<StaticArity<3>>>(StaticArity<3>(), DynamicBinding(false, true, true));
        TestIndexes2<BasicHashTable<DynamicArity>>(DynamicArity(3), StaticBinding<false, true, true>());

        TestIndexes2<OpenHashTable<StaticArity<3>>>(StaticArity<3>(), StaticBinding<false, true, true>());
        TestIndexes2<OpenHashTable<DynamicArity>>(DynamicArity(3), DynamicBinding(false, true, true));
        TestIndexes2<OpenHashTable<StaticArity<3>>>(StaticArity<3>(), DynamicBinding(false, true, true));
        TestIndexes2<OpenHashTable<DynamicArity>>(DynamicArity(3), StaticBinding<false, true, true>());

    }
    
    template<typename Table, typename Arity, typename BindingFB, typename BindingFF, typename BindingBB>
    void TestDeltas1(Arity a, BindingFB fb, BindingFF ff, BindingBB bb)
    {
        Table t(a);
        auto i = t.MakeIndex(fb);
        Enumerator e;
        Int x, y;
        Int row[2];
        bool changed;
        
        // Add some data
        t.Add(changed, 1,2);
        t.Add(changed, 1,3);
        t.Add(changed, 2,3);
        CHECK(changed);
        
        // The table is "empty" until NextIteration()
        t.Find(e, ff);
        CHECK(!t.Next(e,ff,x,y));
        
        // The delta is also empty until NextIteration()
        t.FindDelta(e,ff);
        CHECK(!t.Next(e,ff,x,y));

        // The index is also empty until NextIteration()
        x = 1;
        i.Find(e,fb,x,y);
        CHECK(!i.Next(e,fb,x,y));
        
        t.NextIteration();
        i.NextIteration();

        // Try to find something
        t.Find(e, ff);
        CHECK(t.Next(e,ff,x,y));
        EQUALS(1, x);
        EQUALS(2, y);
        CHECK(t.Next(e,ff,x,y));
        EQUALS(1,x);
        EQUALS(3,y);
        CHECK(t.Next(e,ff,x,y));
        EQUALS(2,x);
        EQUALS(3,y);
        CHECK(!t.Next(e,ff,x,y));
        
        t.FindDelta(e,ff);
        CHECK(t.Next(e,ff,x,y));
        EQUALS(1, x);
        EQUALS(2, y);
        CHECK(t.Next(e,ff,x,y));
        EQUALS(1,x);
        EQUALS(3,y);
        CHECK(t.Next(e,ff,x,y));
        EQUALS(2,x);
        EQUALS(3,y);
        CHECK(!t.Next(e,ff,x,y));

        y = 3;
        i.Find(e,fb,x,y);
        CHECK(i.Next(e,fb,x,y));
        CHECK(x==1 || x==2);
        CHECK(i.Next(e,fb,x,y));
        CHECK(x==1 || x==2);
        CHECK(!i.Next(e,fb,x,y));

        // Add some more data
        changed = false;
        t.Add(changed, 1,0);
        CHECK(changed);
        t.NextIteration();
        i.NextIteration();
        
        t.FindDelta(e,ff);
        CHECK(t.Next(e,ff,x,y));
        EQUALS(x,1);
        EQUALS(y,0);
        CHECK(!t.Next(e,ff,x,y));
    }
    
    void Deltas()
    {
        TestDeltas1<HashTable<StaticArity<2>>>(StaticArity<2>(), StaticBinding<false, true>(), StaticBinding<false, false>(), StaticBinding<true, true>());
        TestDeltas1<HashTable<DynamicArity>>(DynamicArity(2), DynamicBinding(false, true), DynamicBinding(false, false), DynamicBinding(true, true));

        TestDeltas1<DeltaHashTable<StaticArity<2>>>(StaticArity<2>(), StaticBinding<false, true>(), StaticBinding<false, false>(), StaticBinding<true, true>());
        TestDeltas1<DeltaHashTable<DynamicArity>>(DynamicArity(2), DynamicBinding(false, true), DynamicBinding(false, false), DynamicBinding(true, true));

    }

} tt;
