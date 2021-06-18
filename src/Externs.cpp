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

    std::cout << "Registering function " << name[0] << std::endl;
    
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

bool Logical::Call::Get(int index, const char * & value)
{
    return false;
}

void Logical::Call::Set(int index, const char * value)
{
}

void Logical::Call::Set(int index, double value)
{
}

void Logical::Call::YieldResult()
{
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
    
}

void ExternPredicate::Query(Entity *row, Columns columns, Receiver&v)
{
    
}
