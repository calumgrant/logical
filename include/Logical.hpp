/* This is the external API for Logical.
 */

#include <cstdint>

namespace Logical
{
    class Module;

    typedef std::int64_t Int;

    class Call
    {
    public:
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

    protected:
        Call();
        Call(const Call&) = delete;
        Call & operator=(const Call&) = delete;
    };

    enum Direction { In, Out };

    typedef void (*Extern)(Call&);

    class Module
    {
    public:
        void RegisterFunction(Extern, const char *, Direction);
        void RegisterFunction(Extern, const char *, Direction, const char *, Direction);
        void RegisterFunction(Extern, const char *, Direction, const char *, Direction, const char *, Direction);
        void RegisterFunction(Extern, const char *, Direction, const char *, Direction, const char *, Direction, const char *, Direction);
        void RegisterFunction(Extern, int params, const char **, const Direction*);

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
