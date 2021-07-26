#include "Database.hpp"
#include "Colours.hpp"
#include "DatabaseImpl.hpp"
#include "OptimizerImpl.hpp"

#include <iostream>
#include <chrono>

enum class ErrorCode : int
{
    Ok,
    SyntaxError,
    FileNotFound,
    ReportedError,
    InvalidArgs,
    InvalidVersion
};

void PrintBlurb();

int main(int argc, char**argv)
{
    auto startTime = std::chrono::system_clock::now();

    bool supportsTerminal = getenv("TERM");
    
    if(argc==1)
    {
        PrintBlurb();
        return (int)ErrorCode::InvalidArgs;
    }

    int verbosity = 1;
    int errors = 0;
    int optimizationLevel = 1;
    const char * storageFile = nullptr;
    OptimizerImpl optimizer;
    int limitMB = 1000;
    
    for(int i=1; i<argc; ++i)
    {
        if(argv[i][0] == '-')
        {
            switch(argv[i][1])
            {
                case 'v':
                    switch(argv[i][2])
                    {
                        case 0:
                            verbosity = 2;
                            break;
                        case 'v':
                            verbosity = 3;
                            break;
                        default:
                            verbosity = atoi(argv[i]+2);
                            break;
                    }
                    break;
                case 'p':
                    supportsTerminal = false;
                    break;
                case 'q':
                    verbosity = 0;
                    break;
                case 'O':
                    optimizationLevel = atoi(argv[i]+2);
                    break;
                case 'd':
                    storageFile = argv[i]+2;
                    break;
                case 'm':
                    limitMB = atoi(argv[i]+2);
                    // TODO: Check limits here
                    break;
                default:
                    std::cerr << "Unknown option: " << argv[i] << std::endl;
                    return (int)ErrorCode::InvalidArgs;
                    break;
            }
        }
    }
    
    try
    {
        DatabaseImpl db(optimizer, storageFile, limitMB);
        db.SetVerbosity(verbosity);
        optimizer.SetLevel(optimizationLevel);
        db.SetAnsiColours(supportsTerminal);

        for(int i=1; i<argc; ++i)
        {
            if(argv[i][0]=='-') continue;
            if(strcmp(argv[i], "-v")==0)
            {
                continue;
            }
            if(verbosity>1) std::cout << "Reading " << argv[i] << std::endl;

            auto r = db.ReadFile(argv[i]);

            if(r)
            {
                std::cerr << "Failed to read " << argv[i] << std::endl;
                return (int)ErrorCode::FileNotFound;
            }
        }
        
        db.RunQueries();
        
        auto endTime = std::chrono::system_clock::now();
        
        if(verbosity>0)
        {
            std::cout << "Results found    = " << db.NumberOfResults() << std::endl;
            std::cout << "Evaluation steps = " << Database::GlobalCallCount() << std::endl;
            std::cout << "Memory used      = " << db.Storage().size() << " bytes\n";
            std::cout << "Evaluation time  = " << std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count() << " µs\n";
            
            db.CheckErrors();

            if(db.UserErrorReported())
                std::cout << Colours::Error << "Evaluation completed with " << db.NumberOfErrors() << " errors\n" << Colours::Normal;
            else
                std::cout << Colours::Success << "Evaluation completed successfully\n" << Colours::Normal;
        }
        
        if(db.UserErrorReported())
            return (int)ErrorCode::ReportedError;

        return (int)ErrorCode::Ok;

    }
    catch (persist::InvalidVersion v)
    {
        std::cerr << "The specified datafile " << storageFile << " is invalid - please delete it and try again.\n";
        return (int)ErrorCode::InvalidVersion;
    }
}

void PrintBlurb()
{
    std::cout <<
        "Usage: logical <options> <filename> ...\n"
        "Logging options:\n"
        "    -v      Verbose output\n"
        "    -vv     Very verbose output\n"
        "    -q      Quiet output\n"
        "File options:\n"
        "    -       Read from stdin\n"
        "    -oF     Write output to file F\n"
        "    -lF     Log output to file (default: console output)\n"
        "    -dF     Use given datafile F\n"
        "Running options:\n"
        "    -mN     Limit memory usage to N megabytes (default: 1000)\n"
        "    -sN     Limit execution steps to N\n"
        "    -tN     Limit execution time to N seconds (default: 60)\n"
        "Optimization options:\n"
        "    -ON     Set optimization level to N (default: 1)\n"
        "    -fX     Enable optimization X\n"
        "    -fno-X  Disable optimization X\n"
    ;
    
    OptimizerImpl optimizer;
    optimizer.Visit([](Optimization&opt){
        if(opt.level>0)
            std::cout << "    -f" << opt.name << " " << opt.description << " (-O" << opt.level << ")\n";
    });
}
