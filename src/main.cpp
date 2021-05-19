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
    InvalidArgs
};

int main(int argc, char**argv)
{
    if(argc==1)
    {
        std::cout << "Usage: logical <filename> ...\n";
        return (int)ErrorCode::InvalidArgs;
    }

    int verbose = 0;
    bool quiet = false;
    DatabaseImpl db;
    int errors = 0;
    
    auto startTime = std::chrono::system_clock::now();

    for(int i=1; i<argc; ++i)
    {
        if(argv[i][0] == '-')
        {
            switch(argv[i][1])
            {
                case 'v':
                    verbose = true;
                    db.SetVerbose(true);
                    break;
                case 'q':
                    quiet = true;
                    break;
                case 'O':
                    db.SetOptimizationLevel(atoi(argv[i]+2));
                    break;
                default:
                    std::cerr << "Unknown option: " << argv[i] << std::endl;
                    return (int)ErrorCode::InvalidArgs;
                    break;
            }
        }
    }
    
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
        std::cout << "Found " << db.NumberOfResults() << " results\n";
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
