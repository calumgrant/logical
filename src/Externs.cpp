#include <Logical.hpp>
#include "Database.hpp"
#include "DatabaseImpl.hpp"
#include "RelationImpl.hpp"

#include <dlfcn.h>
#include <iostream>

class ModuleImpl : public Logical::Module
{
public:
    ModuleImpl(Database &db) : database(db) { }
    Database &database;
};

class ExternalFunction : public Relation
{
public:
    ExternalFunction(Database & db, Logical::Extern fn, int name, const CompoundName &cn);
};

inline Logical::Module::Module() {}

void Logical::Module::RegisterFunction(Logical::Extern ex, const char * name, Direction direction)
{
    RegisterFunction(ex, 1, &name, &direction);
}

void Logical::Module::RegisterFunction(Logical::Extern ex, int count, const char ** name, const Direction *direction)
{
    auto & db = ((ModuleImpl*)this)->database;
    if(count<1)
    {
        db.Error("Invalid number of parameters for extern");
        return;
    }
    
    auto nameId = db.GetStringId(name[0]);
    std::vector<int> parts;
    parts.reserve(count-1);
    for(int i=1; i<count; ++i)
        parts.push_back(db.GetStringId(name[i]));
    CompoundName cn(parts);
    auto & exfn = db.GetExtern(nameId, cn);
    
    Columns columns(0);
    for(int i=0; i<count; ++i)
        if(direction[i]==Logical::In)
            columns.Bind(i);
    
    exfn.AddExtern(columns, ex);
}

void DatabaseImpl::LoadModule(const char*name)
{
    if(GetVerbosity()>1)
    {
        std::cout << "Loading module " << name << std::endl;
    }

    // This is hacky and needs to change
    std::string n = std::string("lib") + name + ".dylib";
    auto h = dlopen(n.c_str(), RTLD_LOCAL|RTLD_NOW);
    
    if(!h)
    {
        Error("Failed to open shared library");
        return;
    }
    
    auto sym = dlsym(h, "RegisterFunctions");
    if(!sym)
    {
        Error("Module does not contain a RegisterFunctions function");
        return;
    }
    
    auto fn = (void(*)(Logical::Module&))sym;
    
    ModuleImpl module(*this);
    fn(module);
}

ExternPredicate::ExternPredicate(Database & db, int name, const CompoundName &cn) :
    SpecialPredicate(db, name)
{
}

void ExternPredicate::AddExtern(Columns c, Logical::Extern fn)
{
    externs[c] = fn;
}

class CallImpl : public Logical::Call
{
public:
    ModuleImpl module;
    CompoundName &cn;
    Row row;
    Receiver & recv;
    
    CallImpl(Database &db, CompoundName &cn, Row row, Receiver & r) : module(db), cn(cn), recv(r), row(row)
    {
    }
    
    Entity &Index(int index)
    {
        return row[index==0 ? 0 : 1 + cn.parts[index-1]];
    }
    
    void call(Logical::Extern fn)
    {
        try
        {
            fn(*this);
        }
        catch(std::exception & ex)
        {
            module.database.Error(ex.what());
        }
        catch (...)
        {
            module.database.Error("Uncaught exception in extern");
        }
    }
};

void Logical::Call::YieldResult()
{
    auto & call = (CallImpl&)*this;
    call.recv.OnRow(call.row);
}

bool Logical::Call::Get(int index, const char * & value)
{
    auto & call = (CallImpl&)*this;
    auto e = call.Index(index);
    if(e.IsString())
    {
        value = call.module.database.GetString((std::int64_t)e).c_str();
        return true;
    }
    return false;
}

bool Logical::Call::GetAtString(int index, const char * & value)
{
    auto & call = (CallImpl&)*this;
    auto e = call.Index(index);
    if(e.IsAtString())
    {
        value = call.module.database.GetAtString((std::int64_t)e).c_str();
        return true;
    }
    return false;
}


bool Logical::Call::Get(int index, double & value)
{
    auto & call = (CallImpl&)*this;
    auto e = call.Index(index);
    if(e.IsFloat())
    {
        value = (double)e;
        return true;
    }
    return false;
}

bool Logical::Call::Get(int index, long & value)
{
    auto & call = (CallImpl&)*this;
    auto e = call.Index(index);
    if(e.IsInt())
    {
        value = (std::int64_t)e;
        return true;
    }
    return false;
}

bool Logical::Call::Get(int index, bool & value)
{
    auto & call = (CallImpl&)*this;
    auto e = call.Index(index);
    if(e.IsBool())
    {
        value = (std::int64_t)e;
        return true;
    }
    return false;
}


void Logical::Call::Set(int index, const char * value)
{
}

void Logical::Call::Set(int index, double value)
{
    auto & call = (CallImpl&)*this;
    call.Index(index) = value;
}

Logical::Call::Call()
{}

class ChainReceiver : public Receiver
{
public:
    ChainReceiver(Receiver & r, int arity, Columns mask1, Columns mask2, Row row1, Row row2) :
        r(r), arity(arity), mask1(mask1), mask2(mask2), row1(row1), row2(row2)
    {
    }
    
    Receiver & r;
    int arity;
    Columns mask1, mask2;
    Row row1, row2;
    
    void OnRow(Row row)
    {
        for(int i=0; i<arity; ++i)
            if(!mask1.IsBound(i))
            {
                if(mask2.IsBound(i))
                {
                    if(row1[i] != row2[i]) return;
                }
                else
                    row2[i] = row1[i];
            }
        r.OnRow(row2);
    }
};

void ExternPredicate::Query(Row row, Columns columns, Receiver & r)
{
    auto fn = externs.find(columns);
    
    if(fn != externs.end())
    {
        CallImpl call(database, compoundName, row, r);
        call.call(fn->second);
        return;
    }

    for(auto & e : externs)
    {
        if(e.first <= columns)
        {
            // Can dispatch here instead.
            int arity = compoundName.parts.size()+1;
            Entity tmp[arity];
            for(int i=0; i<arity; ++i)
                if(e.first.IsBound(i))
                    tmp[i] = row[i];
            ChainReceiver rec(r, arity, e.first, columns, tmp, row);
            CallImpl call(database, compoundName, tmp, rec);
            call.call(e.second);
            return;
        }
    }

    database.Error("Cannot bind to extern");
}

class NullReceiver : public Receiver
{
public:
    void OnRow(Row row) { }
};

void ExternPredicate::Add(const Entity * row)
{
    int arity = 1 + compoundName.parts.size();
    Columns columns((1 <<arity)-1);
    
    auto fn = externs[columns];
    
    if(fn)
    {
        NullReceiver r;
        CallImpl call(database, compoundName, (Entity*)row, r);
        call.call(fn);
    }
    else
    {
        database.Error("Cannot bind to extern");
    }

}

Logical::Module & Logical::Call::GetModule()
{
    auto & call = (CallImpl&)*this;
    return call.module;
}

void Logical::Module::ReportError(const char * str)
{
    auto & db = ((ModuleImpl*)this)->database;
    db.Error(str);
}
