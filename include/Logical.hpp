/* This is the external API for Logical.
 */

namespace Logical
{
    class Module;

    class Call
    {
    public:
        void YieldResult();

        bool Get(int index, long & value);
        void Set(int index, long value);

        bool Get(int index, const char *& value);
        void Set(int index, const char * value);

        bool GetAtString(int index, const char *& value);
        void SetAtString(int index, const char * value);

        bool Get(int index, bool & value);
        void Set(int index, bool value);

        bool Get(int index, double & value);
        void Set(int index, double value);

        Call(const Call&) = delete;
        Call & operator=(const Call&) = delete;

        Module & GetModule();
    };

    enum Direction { In, Out };

    typedef void (*Extern)(Call&);

    class Module
    {
    public:
        Module(const Module&) = delete;
        Module & operator=(const Module&) = delete;

        void RegisterFunction(Extern, const char *, Direction);
        void RegisterFunction(Extern, const char *, Direction, const char *, Direction);
        void RegisterFunction(Extern, const char *, Direction, const char *, Direction, const char *, Direction);
        void RegisterFunction(Extern, const char *, Direction, const char *, Direction, const char *, Direction, const char *, Direction);
        void RegisterFunction(Extern, int params, const char **, const Direction*);

        void LoadModule(const char*);
        void ReportError(const char*);
        void LoadFile(const char*);
    };
}
