#include "Database.hpp"
#include "Colours.hpp"
#include "DatabaseImpl.hpp"

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
    bool supportsTerminal = getenv("TERM");
    
    if(argc==1)
    {
        PrintBlurb();
        return (int)ErrorCode::InvalidArgs;
    }

    int verbose = 0;
    bool quiet = false;
    int errors = 0;
    int optimizationLevel = 1;
    const char * storageFile = nullptr;
    
    auto startTime = std::chrono::system_clock::now();

    for(int i=1; i<argc; ++i)
    {
        if(argv[i][0] == '-')
        {
            switch(argv[i][1])
            {
                case 'v':
                    verbose = true;
                    break;
                case 'p':
                    supportsTerminal = false;
                    break;
                case 'q':
                    quiet = true;
                    break;
                case 'O':
                    optimizationLevel = atoi(argv[i]+2);
                    break;
                case 'd':
                    storageFile = argv[i]+2;
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
        DatabaseImpl db(storageFile, 1000);
        db.SetVerbose(verbose);
        db.SetOptimizationLevel(optimizationLevel);
        db.SetAnsiColours(supportsTerminal);

        for(int i=1; i<argc; ++i)
        {
            if(argv[i][0]=='-') continue;
            if(strcmp(argv[i], "-v")==0)
            {
                continue;
            }
            if(verbose) std::cout << "Reading " << argv[i] << std::endl;

            auto r = db.ReadFile(argv[i]);

            if(r)
            {
                std::cerr << "Failed to read " << argv[i] << std::endl;
                return (int)ErrorCode::FileNotFound;
            }
        }
        
        db.RunQueries();
        
        auto endTime = std::chrono::system_clock::now();
        
        if(!quiet)
        {
            std::cout << "Found " << db.NumberOfResults() << " results in total\n";
            std::cout << "Evaluation steps = " << Database::GlobalCallCount() << std::endl;
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
        "Options include:\n"
        "    -v      Verbose output\n"
        "    -vv     Very verbose output\n"
        "    -q      Quiet output\n"
        "    -       Read from stdin\n"
        "    -oF     Write output to file F\n"
        "    -F      Log output to file\n"
        "    -dF     Use given datafile F\n"
        "    -mN     Limit memory usage to N megabytes (default: 1000)\n"
        "    -sN     Limit execution steps to N\n"
        "    -tN     Limit execution time to N seconds (default: 60)\n"
        "    -ON     Set optimization level 0,1 or 2. (default: 1)\n"
        "    -fX     Enable optimization X\n"
        "    -fno-X  Disable optimization X\n"
    ;
}
