#include <Logical.hpp>
#include "Database.hpp"
#include "DatabaseImpl.hpp"
#include "RelationImpl.hpp"

#include <dlfcn.h>
#include <iostream>
#include <filesystem>

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

static PredicateName GetPredicate(Database & db, const std::initializer_list<const char*> & name)
{
    auto i = name.begin();
    auto nameId = db.GetStringId(*i);
    std::vector<int> parts;
    parts.reserve(name.size()-1);
    for(++i; i!=name.end(); ++i)
        parts.push_back(db.GetStringId(*i));
    CompoundName cn(parts);
    
    PredicateName pn;
    pn.arity = name.size();
    pn.objects.parts.push_back(nameId);
    pn.attributes = std::move(cn);

    return pn;
}

void Logical::Module::AddFunction(Logical::Extern ex, const std::initializer_list<const char*> & name, const std::initializer_list<Mode> & direction, void * data)
{
    auto & db = ((ModuleImpl*)this)->database;
    if(name.size()<1)
    {
        db.Error("Invalid number of parameters for extern");
        return;
    }

    if(name.size()!= direction.size())
    {
        db.Error("Invalid name/mode count extern");
        return;
    }
    
    auto pn = ::GetPredicate(db, name);
        
    Columns columns(0);
    int i=0;
    bool varargs = false;
    for(auto m : direction)
    {
        if(m==Logical::In)
            columns.Bind(i);
        i++;
        varargs = m==Varargs;
    }
    
    if(varargs)
    {
        db.Addvarargs(pn.objects.parts[0], ex, data);
    }
    else
    {
        auto & exfn = db.GetExtern(pn);
        exfn.AddExtern(columns, ex, data);
    }
}


void Logical::Module::AddCommand(Extern ex, const std::initializer_list<const char*> & name, void * data)
{
    auto & db = ((ModuleImpl*)this)->database;
    
    PredicateName pn = ::GetPredicate(db, name);
    
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
    bool first = false, last = false;
    
    CallImpl(Database &db, const PredicateName & name, Columns columns, Row row, Receiver & r, void * data) : module(db), name(name), columns(columns), setColumns(columns), recv(r), row(row), data(data)
    {
    }
    
    int IndexMap(int index) const
    {
        if(index<0 || index >= name.arity)
            throw std::logic_error("Invalid argument position");
        if(index > name.attributes.mapFromInputToOutput.size()) return index;
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
        call.Error("Cannot call YieldResult until all columns are set");
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
        value = call.module.database.GetString((std::int64_t)e);
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
        value = call.module.database.GetAtString((std::int64_t)e);
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
        call.Error("Invalid argument");
        return;
    }
    
    call.setColumns.Bind(index);
    
    auto & e = call.Index(index);
    e = ::Entity(EntityType::String, call.module.database.GetStringId(value));
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
    call.Index(index) = ::Entity(EntityType::Integer, (std::int64_t)value);
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

    database.Error(r.location, "Arguments to this extern must be bound");
}

class NullReceiver : public Receiver
{
public:
    void OnRow(Row row) { }
};

bool ExternPredicate::Add(const SourceLocation & loc, const Entity * row)
{
    if(writer.fn)
    {
        NullReceiver r;
        r.location = loc;
        Columns allSet((1<<name.arity)-1);
        CallImpl call(database, name, allSet, (Entity*)row, r, writer.data);
        call.call(writer.fn);
        return true;
    }
    else
    {
        database.Error(loc, "This extern does not support writes");
        return false;
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
        call.Error("Invalid argument");
        return "<error>";
    }
    
    i = call.name.MapArgument(i);
    assert(call.name.objects.parts.size()==1);
    
    if(i>call.name.attributes.parts.size()) return "";
    int id = i==0 ? call.name.objects.parts[0] : call.name.attributes.parts[i-1];
    return call.module.database.GetString(id);
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

void Logical::Module::SetMemoryLimitMB(Int expected)
{
    auto & db = ((ModuleImpl*)this)->database;
    db.SetMemoryLimit(expected * 1024 * 1024);
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
    index = call.name.MapArgument(index);
    return call.columns.IsBound(index) ? In : Out;
}

void ExternPredicate::AddVarargs(Logical::Extern fn, void * data)
{
    varargs.fn = fn;
    varargs.data = data;
    writer.fn = fn;
    writer.data = data;
}

Logical::Call & Logical::Module::GetPredicate(const std::initializer_list<const char*> & name)
{
    auto & db = ((ModuleImpl*)this)->database;
    
    PredicateName pn = ::GetPredicate(db, name);
    auto & relation = db.GetRelation(pn);
    return relation.GetExternalCall();
}

class ReceiverCallImpl : public CallImpl, public Receiver
{
public:
    ReceiverCallImpl(Relation & rel) :
        CallImpl(rel.GetDatabase(), rel.name, 0, nullptr, *this, nullptr),
        relation(rel), rowData(name.arity)
    {
        row = rowData.data();
    }
    
    Relation & relation;
    std::vector<Entity> rowData;
    
    void OnRow(Row row) override
    {
        relation.Add(row);
    }
};

Logical::Call & Relation::GetExternalCall()
{
    // !! Beware: this isn't threadsafe/reentrant
    if(!externalCall)
    {
        externalCall = std::make_shared<ReceiverCallImpl>(*this);
        allowEmpty = true;
    }

    return *externalCall;
}

Logical::Entity Logical::Module::NewObject()
{
    auto & db = ((ModuleImpl*)this)->database;
    return db.NewEntity();
}

void Logical::Call::Set(int index, Entity e)
{
    auto & call = (CallImpl&)*this;
    call.setColumns.Bind(index);
    call.Index(index) = e;
}

void Logical::Call::Finalize()
{
    auto rel = dynamic_cast<ReceiverCallImpl*>(this);
    if(rel)
    {
        rel->relation.Finalize();
    }
    else
    {
        auto & call = (CallImpl&)*this;
        call.module.database.Error("Invalid call type to Finalize()");
    }
}

Logical::Call::~Call() { }

void Logical::Call::ErrorInsert(const char * msg)
{
    std::cerr << msg;
}

void Logical::Call::BuildError()
{
    std::cerr << "\n" << Colours::Normal;
}

void Logical::Call::ReportError()
{
    auto & call = (CallImpl&)*this;
    call.module.database.ReportUserError();
    auto & loc = call.recv.location;
    std::cerr << Colours::Error << "Error in call from " << call.module.database.GetString(loc.filenameId)
        << ":" << loc.line << ":" << loc.column << ": ";
}

void Logical::Call::Import(const char * name)
{
    auto & call = (CallImpl&)*this;

    std::string filename = name;
    filename += ".dl";

    // auto absFile = std::filesystem::absolute(filename);
    
    if(0 == call.module.database.ReadFile(filename.c_str()))
        return;

    std::filesystem::path callingFile = call.module.database.GetString(call.recv.location.filenameId);
    // callingFile = std::filesystem::absolute(callingFile);

    callingFile.remove_filename();
    std::filesystem::path p = name;
    p+=".dl";
    auto module = callingFile / p;
    
    if(0 == call.module.database.ReadFile(module.c_str()))
        return;

    call.Error("Failed to import ", name);
}

bool ExternPredicate::Add(const Entity * row)
{
    assert(0);
    return false;
}

void ExternPredicate::OnStartRunningRules()
{
    if(varargs.fn)
    {
        NullReceiver recv;
        CallImpl call(database, name, {0}, nullptr, recv, varargs.data);
        call.first = true;
        varargs.fn(call);
    }
}

void ExternPredicate::OnStopRunningRules()
{
    if(varargs.fn)
    {
        NullReceiver recv;
        CallImpl call(database, name, {0}, nullptr, recv, varargs.data);
        call.last = true;
        varargs.fn(call);
    }
}

bool Logical::Call::First() const
{
    auto & call = (CallImpl&)*this;
    return call.first;
}

bool Logical::Call::Last() const
{
    auto & call = (CallImpl&)*this;
    return call.last;
}

void Logical::Call::CountResult()
{
    auto & call = (CallImpl&)*this;
    call.module.database.AddResult();
}
