#include "Relation.hpp"
#include "Database.hpp"
#include "RelationImpl.hpp"

#include "TableImpl.hpp"

#include <iostream>

TablePredicate::TablePredicate(Database &db, const CompoundName & cn, int arity) :
    Predicate(db, cn.parts[0]), arity(arity),
    hash({}, 100, Comparer(data, arity, -1), Comparer(data, arity, -1)),
    name(cn)
{
}

TablePredicate::TablePredicate(Database &db, RelationId name, int arity) :
    Predicate(db, name), arity(arity),
    hash({}, 100, Comparer(data, arity, -1), Comparer(data, arity, -1)),
    name()
{
    
}

void TablePredicate::Add(const Entity *row)
{
    /*
    std::cout << "Writing (";
    for(int i=0; i<arity; ++i)
    {
        if(i>0) std::cout << ", ";
        database.PrintQuoted(row[i], std::cout);
    }
    std::cout << ") into " << database.GetString(Name()) << std::endl;
     */
    
    for(int i=0; i<arity; ++i)
        assert(row[i].type != EntityType::None);
    
    auto s = data.size();
    data.insert(data.end(), row, row+arity);
    
    auto i = hash.insert(s);
            
    if(!i.second)
    {
        // It was duplicated, so remove it.
        data.resize(s);
    }
    else if(loop)
    {
        ++loop->numberOfResults;
    }
}

bool TablePredicate::NextIteration()
{
    bool moreResults = deltaEnd < data.size();
    
    for(std::size_t s = deltaStart; s < deltaEnd; s+= arity)
    {
        for(auto & index : indexes)
        {
            index.second.insert(s);
        }
    }
    deltaStart = deltaEnd;
    deltaEnd = data.size();
    return moreResults;
}

TablePredicate::map_type & TablePredicate::GetIndex(int mask)
{
    auto i = indexes.find(mask);
    if(i != indexes.end())
    {
        return i->second;
    }
    
    // Create an index
    
    map_type it({}, 100, Comparer(data, arity, mask), Comparer(data, arity, mask));
    
    auto &index = indexes.insert(std::make_pair(mask, it)).first->second;
    
    for(auto i=0; i<deltaStart; i+=arity)
    {
        index.insert(i);
    }
    
    return index;
}

void TablePredicate::Query(Entity * row, int mask, Receiver&v)
{
    if(!HasRules() && data.empty())
    {
        database.WarningEmptyRelation(*this);
        return;
    }
    
    RunRules();
    
    if(mask==0)
    {
        // Unclear if we need reentrancy guard here.
        auto r = reentrancy.Enter();
        for(std::size_t s=0; s<data.size(); s+=arity)
            v.OnRow(&data[s]);
    }
    else
    {
        
        if(mask == -1 || mask == (1<<arity)-1)
        {
            auto s = data.size();
            data.insert(data.end(), row, row+arity);
            auto i = hash.find(s);
            data.resize(s);
            
            if (i != hash.end() && *i<deltaStart)
                v.OnRow(&data[*i]);
            return;
        }
        else
        {
            auto & index = GetIndex(mask);
            auto s = data.size();
            data.insert(data.end(), row, row+arity);
            auto result = index.equal_range(s);
            data.resize(s);

            auto r = reentrancy.Enter();

            for(auto i = result.first; i!=result.second; ++i)
            {
                int n = *i;  // Debug info
                assert(n>=0 && n < data.size());
                v.OnRow(&data[*i]);
            }
        }
    }    
}

int TablePredicate::Arity() const
{
    return arity;
}

const CompoundName * TablePredicate::GetCompoundName() const
{
    return name.parts.empty() ? nullptr : &name;
}

void TablePredicate::QueryDelta(Entity * row, int columns, Receiver &v)
{
    RunRules();
    
    if(deltaStart == deltaEnd)
    {
        deltaStart = 0;
        deltaEnd = data.size();
    }
    
    if(columns==0)
    {
        // This is an optimization on the next part
        // to make it slightly faster.
        for(std::size_t s=deltaStart; s<deltaEnd; s += arity)
        {
             v.OnRow(&data[s]);
        }
        return;
    }
    
    for(std::size_t s=deltaStart; s<deltaEnd; s += arity)
    {
        bool found = true;
        for(int i=0; i<arity; ++i)
            if(columns & (1<<i) && data[s+i] != row[i])
            {
                found = false;
                break;
            }
        if(found)
            v.OnRow(&data[s]);
    }
}

void TablePredicate::FirstIteration()
{
    deltaStart = 0;
    deltaEnd = data.size();
}

void UnaryTable::Add(const Entity *row)
{
    MakeDirty();
    //std::cout << "Added (" << (int)e.type << "," << e.i << ") to the table\n";
    values.insert(row[0]);
}

void BinaryTable::Add(const Entity * row)
{
    MakeDirty();
    
    auto value = std::make_pair(row[0],row[1]);
    //std::cout << "Added (" << (int)e1.type << "," << e1.i << ") (" << (int)e2.type << "," << e2.i << ") to the table\n";
    auto p = values.insert(value);
    
    if(p.second)
    {
        // TODO: Lazy map initialization
        map1.insert(value);
        map2.insert(std::make_pair(row[1], row[0]));
    }
}

std::size_t UnaryTable::Count()
{
    return values.size();
}

std::size_t BinaryTable::Count()
{
    return values.size();
}

std::size_t TablePredicate::Count()
{
    return hash.size();
}

void UnaryTable::Query(Entity * row, int columns, Receiver &v)
{
    RunRules();
    
    switch(columns)
    {
        case 0:
            for(auto &i : values)
            {
                v.OnRow(const_cast<Entity*>(&i));
            }
            break;
        case 1:
            {
                auto i = values.find(row[0]);
                if (i != values.end())
                    v.OnRow(row);
                break;
            }
            break;
        default:
            assert(!"Invalid query columns");
    }
}

void UnaryTable::QueryDelta(Entity * row, int columns, Receiver &v)
{
    Query(row, columns, v);
}

void BinaryTable::Query(Entity * row, int bound, Receiver&v)
{
    RunRules();
    
    switch(bound)
    {
        case 0:
            for(auto & i : values)
            {
                Entity data[2];
                // TODO: Store a pair in the table to make the scan faster
                data[0] = i.first;
                data[1] = i.second;
                v.OnRow(data);
            }
            break;
        case 1:
            {
                auto range = map1.equal_range(row[0]);
                for(auto i=range.first; i!=range.second; ++i)
                {
                    row[1] = i->second;
                    v.OnRow(row);  // ??
                }
            }
            break;
        case 2:
            {
                auto range = map2.equal_range(row[1]);
                for(auto i=range.first; i!=range.second; ++i)
                {
                    row[0] = i->second;
                    v.OnRow(row);  // ??
                }
            }
            break;
        case 3:
            {
                if(values.find(std::make_pair(row[0], row[1])) != values.end())
                    v.OnRow(row);
            }
            break;
        default:
            std::cout << "TODO: Implement the join\n";
            // Not implemented yet
            break;
    }
    // todo
}

void BinaryTable::QueryDelta(Entity * row, int columns, Receiver &v)
{
    Query(row, columns, v);
}

UnaryTable::UnaryTable(Database &db, int name) : Predicate(db, name)
{
}

BinaryTable::BinaryTable(Database &db, int name) : Predicate(db, name)
{
}

TablePredicate::Comparer::Comparer(const std::vector<Entity> & base, int arity, int mask) : base(base), arity(arity), mask(mask)
{
}

int UnaryTable::Arity() const
{
    return 1;
}

int BinaryTable::Arity() const
{
    return 2;
}

bool UnaryTable::NextIteration()
{
    return false;
}

bool BinaryTable::NextIteration()
{
    return false;
}


void UnaryTable::FirstIteration()
{
}

void BinaryTable::FirstIteration()
{
}
