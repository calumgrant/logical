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

void Logical::Module::AddFunction(Logical::Extern ex, const char * name, Mode direction)
{
    AddFunction(ex, 1, &name, &direction, nullptr);
}

void Logical::Module::AddFunction(Logical::Extern ex, const char * name1, Mode direction1, const char * name2, Mode direction2)
{
    const char *names[] = { name1, name2 };
    Mode dirs[] = { direction1, direction2 };
    AddFunction(ex, 2, names, dirs, nullptr);
}

void Logical::Module::AddFunction(Logical::Extern ex, const char * name1, Mode direction1, const char * name2, Mode direction2, const char * name3, Mode direction3)
{
    const char *names[] = { name1, name2, name3 };
    Mode dirs[] = { direction1, direction2, direction3 };
    AddFunction(ex, 3, names, dirs, nullptr);
}

void Logical::Module::AddFunction(Logical::Extern ex, const char * name1, Mode direction1, const char * name2, Mode direction2, const char * name3, Mode direction3, const char * name4, Mode direction4)
{
    const char *names[] = { name1, name2, name3, name4 };
    Mode dirs[] = { direction1, direction2, direction3, direction4 };
    AddFunction(ex, 4, names, dirs, nullptr);
}


void Logical::Module::AddFunction(Logical::Extern ex, int count, const char ** name, const Mode *direction, void * data)
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
    PredicateName pn;
    pn.objects.parts.push_back(nameId);
    pn.attributes = cn;
    pn.arity = count;
    
    Columns columns(0);
    for(int i=0; i<count; ++i)
        if(direction[i]==Logical::In)
            columns.Bind(i);
    
    if(direction[count-1] == Varargs)
    {
        db.Addvarargs(nameId, ex, data);
    }
    else
    {
        auto & exfn = db.GetExtern(pn);
        exfn.AddExtern(columns, ex, data);
    }
}

void Logical::Module::AddCommand(Extern ex, const char*name)
{
    AddCommand(ex, 1, &name, nullptr);
}

void Logical::Module::AddCommand(Extern ex, const char*name1, const char * name2)
{
    const char * names[] = { name1, name2 };
    AddCommand(ex, 2, names, nullptr);
}

void Logical::Module::AddCommand(Extern ex, const char*name1, const char * name2, const char * name3)
{
    const char * names[] = { name1, name2, name3 };
    AddCommand(ex, 3, names, nullptr);
}


void Logical::Module::AddCommand(Extern ex, int count, const char ** name, void * data)
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
    PredicateName pn;
    pn.arity = count;
    pn.objects.parts.push_back(nameId);
    pn.attributes = std::move(cn);
    auto & exfn = db.GetExtern(pn);
    
    exfn.AddExtern(ex, data);
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

ExternPredicate::ExternPredicate(Database & db, const PredicateName &name) :
    SpecialPredicate(db, name)
{
}

void ExternPredicate::AddExtern(Columns c, Logical::Extern fn, void * data)
{
    externs[c] = ExternFn { fn, data };
}

void ExternPredicate::AddExtern(Logical::Extern fn, void * data)
{
    writer = ExternFn { fn, data };
}

class CallImpl : public Logical::Call
{
public:
    ModuleImpl module;
    const PredicateName &name;
    Columns columns;
    Columns setColumns;
    Row row;
    Receiver & recv;
    void * data;
    
    CallImpl(Database &db, const PredicateName & name, Columns columns, Row row, Receiver & r, void * data) : module(db), name(name), columns(columns), setColumns(columns), recv(r), row(row), data(data)
    {
    }
    
    int IndexMap(int index) const
    {
        if(index<0 || index > name.attributes.mapFromInputToOutput.size())
            throw std::logic_error("Invalid argument position");
        return index==0 ? 0 : 1 + name.attributes.mapFromInputToOutput[index-1];
    }
    
    Entity &Index(int index)
    {
        return row[IndexMap(index)];
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

    if(!call.setColumns.IsFullyBound(call.name.arity))
        call.module.ReportError("Cannot call YieldResult until all columns are set");
    call.recv.OnRow(call.row);
}

void * Logical::Call::GetData()
{
    auto & call = (CallImpl&)*this;
    return call.data;
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

bool Logical::Call::Get(int index, Logical::Int & value)
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
    auto & call = (CallImpl&)*this;
    
    if(index>=call.name.arity)
    {
        call.module.ReportError("Invalid argument");
        return;
    }
    
    call.setColumns.Bind(index);
    
    auto & e = call.Index(index);
    e = Entity(EntityType::String, call.module.database.GetStringId(value));
}

void Logical::Call::Set(int index, double value)
{
    auto & call = (CallImpl&)*this;
    call.Index(index) = value;
    call.setColumns.Bind(index);
}

void Logical::Call::Set(int index, Logical::Int value)
{
    auto & call = (CallImpl&)*this;
    call.Index(index) = Entity(EntityType::Integer, (std::int64_t)value);
    call.setColumns.Bind(index);
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
    if(varargs.fn)
    {
        CallImpl call(database, name, columns, row, r, varargs.data);
        call.call(varargs.fn);
        return;
    }
    
    auto fn = externs.find(columns);
    
    if(fn != externs.end())
    {
        CallImpl call(database, name, columns, row, r, fn->second.data);
        call.call(fn->second.fn);
        return;
    }

    for(auto & e : externs)
    {
        if(e.first <= columns)
        {
            // Can dispatch here instead.
            int arity = name.arity;
            Entity tmp[arity];
            for(int i=0; i<arity; ++i)
                if(e.first.IsBound(i))
                    tmp[i] = row[i];
            ChainReceiver rec(r, arity, e.first, columns, tmp, row);
            CallImpl call(database, name, e.first, tmp, rec, e.second.data);
            call.call(e.second.fn);
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
    if(writer.fn)
    {
        NullReceiver r;
        Columns allSet((1<<name.arity)-1);
        CallImpl call(database, name, allSet, (Entity*)row, r, writer.data);
        call.call(writer.fn);
    }
    else
    {
        database.Error("This extern does not support writes");
    }
}

Logical::Module & Logical::Call::GetModule()
{
    auto & call = (CallImpl&)*this;
    return call.module;
}

int Logical::Call::ArgCount() const
{
    auto & call = (CallImpl&)*this;
    return call.name.arity;
}

const char * Logical::Call::ArgName(int i) const
{
    auto & call = (CallImpl&)*this;
    if(i>=call.name.arity)
    {
        call.module.ReportError("Invalid argument");
        return "<error>";
    }
    
    assert(call.name.objects.parts.size()==1);
    int id = i==0 ? call.name.objects.parts[0] : call.name.attributes.parts[i-1];
    return call.module.database.GetString(id).c_str();
}

void Logical::Module::ReportError(const char * str)
{
    auto & db = ((ModuleImpl*)this)->database;
    db.Error(str);
}

void Logical::Module::SetExpectedResults(Int expected)
{
    auto & db = ((ModuleImpl*)this)->database;
    db.SetExpectedResults(expected);
}

void Logical::Module::SetEvaluationStepLimit(Int limit)
{
    auto & db = ((ModuleImpl*)this)->database;
    db.SetEvaluationLimit(limit);
}

void Logical::Module::LoadModule(const char * name)
{
    auto & db = ((ModuleImpl*)this)->database;
    db.LoadModule(name);
}

Logical::Mode Logical::Call::GetMode(int index) const
{
    auto & call = (CallImpl&)*this;
    return call.columns.IsBound(index) ? In : Out;
}

void ExternPredicate::AddVarargs(Logical::Extern fn, void * data)
{
    varargs.fn = fn;
    varargs.data = data;
}
