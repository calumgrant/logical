/* This is the external API for Logical.
 */

#include <cstdint>

namespace Logical
{
    class Module;

    typedef std::int64_t Int;

    // The Call object is passed to every extern, used primarily to
    // read/write the arguments to the call, and to signal the return of results.
    class Call
    {
    public:
        // Yield a result from a call, using the values previously set using Set.
        // This can be called no times (failure/no result), one times or many times (multiple results).
        void YieldResult();

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

        Module & GetModule();
        
        // Gets the data previously passed to RegisterFunction().
        // This is an opaque payload.
        void * GetData();

    protected:
        Call();
        Call(const Call&) = delete;
        Call & operator=(const Call&) = delete;
    };

    enum Mode { In, Bound=In, Out, Unbound=Out, Write };

    // The type of all external functions.
    typedef void (*Extern)(Call&);

    // The module object used primarily to register the externs in the module.
    class Module
    {
    public:
        void RegisterFunction(Extern, const char *, Mode);
        void RegisterFunction(Extern, const char *, Mode, const char *, Mode);
        void RegisterFunction(Extern, const char *, Mode, const char *, Mode, const char *, Mode);
        void RegisterFunction(Extern, const char *, Mode, const char *, Mode, const char *, Mode, const char *, Mode);
        void RegisterFunction(Extern, int params, const char **, const Mode*, void * data);

        void LoadModule(const char*);
        void ReportError(const char*);
        void LoadFile(const char*);
        
        void SetExpectedResults(Int expected);
        void SetEvaluationStepLimit(Int limit);
        
    protected:
        Module();
        Module(const Module&) = delete;
        Module & operator=(const Module&) = delete;
    };
}

extern "C" void RegisterFunctions(Logical::Module&);
