#pragma once

/* This is the external API for Logical.
 */

#include <cstdint>
#include <initializer_list>

namespace Logical
{
    class Module;

    typedef std::int64_t Int;
    typedef std::uint32_t Index;

    struct Entity  // !! Rename to Value
    {
        std::uint64_t value;
    };

    enum Mode { In, Bound=In, Out, Unbound=Out, Varargs };

    // The Call object is passed to every extern, used primarily to
    // read/write the arguments to the call, and to signal the return of results.
    class Call
    {
    public:
        // Yield a result from a call, using the values previously set using Set.
        // This can be called no times (failure/no result), one time (single result),
        // or many times (multiple results).
        void YieldResult();

        void Get(int index, Entity & value);
        void Set(int index, Entity value);

        bool Get(int index, Int & value);
        void Set(int index, Int value);

        bool Get(int index, const char *& value);
        void Set(int index, const char * value);

        bool GetAtString(int index, const char *& value);
        void SetAtString(int index, const char * value);

        bool Get(int index, bool & value);
        void Set(int index, bool value);

        bool Get(int index, double & value);
        void Set(int index, double value);
        
        void SetNewObject(int index);

        Module & GetModule();
        
        // Gets the data previously passed to RegisterFunction().
        // This is an opaque payload.
        void * GetData();
        
        int ArgCount() const;
        Mode GetMode(int index) const;
        const char * ArgName(int index) const;
        
        void Finalize();
        
        template<typename...Args>
        void Error(Args... args)
        {
            ReportError();
            BuildError(args...);
        }
        
        void Import(const char* moduleName);
        
        bool First() const;
        bool Last() const;

    protected:
        void ReportError();
        void BuildError();
        void ErrorInsert(const char*);
        
        template<typename Arg, typename...Args>
        void BuildError(Arg a, Args... b)
        {
            ErrorInsert(a);
            BuildError(b...);
        }
        
        Call();
        virtual ~Call();
        Call(const Call&) = delete;
        Call & operator=(const Call&) = delete;
    };

    // The type of all external functions.
    typedef void (*Extern)(Call&);

    // The module object used primarily to register the externs in the module.
    class Module
    {
    public:
        
        // Commands are externs where all parameters are in/bound
        // and they are used in the `then` part of a rule to change the state.
        // They are predicates with side-effects.
        void AddCommand(Extern, const std::initializer_list<const char*> & name, void * data=nullptr);
        
        // Functions are regular externs that compute or check something.
        // They do not generally have side-effects, although they could cache data internally.
        void AddFunction(Extern, const std::initializer_list<const char*> & params, const std::initializer_list<Mode> & modes, void * data=nullptr);

        // Tables are externs where all parameters are out/unbound
        // and the results are cached. This is useful for reading datafiles for example.
        void AddTable(Extern, const std::initializer_list<const char*> & name, void * data=nullptr);
        
        // Reads all data from a given predicate and passes it into Extern.
        void Read(Extern, const char*);
        void Read(Extern, int params, const std::initializer_list<const char*> & name, void * data);

        void LoadModule(const char*);
        void ReportError(const char*);
        void LoadFile(const char*);
        
        void SetExpectedResults(Int expected);
        void SetEvaluationStepLimit(Int limit);
        void SetMemoryLimitMB(Int limit);
        Call & GetPredicate(const std::initializer_list<const char*> &);

        Entity NewObject();
        Entity GetString(const char * string);
        
    protected:
        Module();
        Module(const Module&) = delete;
        Module & operator=(const Module&) = delete;
    };
}

extern "C" void RegisterFunctions(Logical::Module&);
