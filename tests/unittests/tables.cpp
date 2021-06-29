
#include "../../include/Table.hpp"
#include "../../external/simpletest/simpletest.hpp"

class TablesTest : public Test::Fixture<TablesTest>
{
public:
    TablesTest() : base("Tables")
    {
        AddTest(&TablesTest::SimpleTable);
    }
    
    void SimpleTable()
    {
        Logical::SimpleTable<Logical::FixedArity<1>> t1;
        
        EQUALS(0, t1.size());
        
        t1.Add(0);
        t1.Add(3);
        t1.Add(2);
        t1.Add(0);
        EQUALS(4, t1.size());
                
        t1.Finalize();
        EQUALS(3, t1.size());
    }
} tt;
