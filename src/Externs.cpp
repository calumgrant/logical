#include <Logical.hpp>
#include "Database.hpp"
#include "DatabaseImpl.hpp"

#include <dlfcn.h>
#include <iostream>

inline Logical::Module::Module() {}

void Logical::Module::RegisterFunction(Logical::Extern ex, const char * name, Direction d)
{
    std::cout << "Registering function " << name << std::endl;
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

class ModuleImpl : public Logical::Module
{
public:
    ModuleImpl(Database &db) : database(db) { }
    Database &database;
};

void DatabaseImpl::LoadModule(const char*name)
{
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
    }
    
    auto fn = (void(*)(Logical::Module&))sym;
    
    ModuleImpl module(*this);
    fn(module);
}
