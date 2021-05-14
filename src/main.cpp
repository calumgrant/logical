#include <iostream>
#include "Database.hpp"
#include <chrono>

int main(int argc, char**argv)
{
    if(argc==1)
    {
        std::cout << "Usage: logical <filename> ...\n";
        return 127;
    }

    int verbose=0;
    Database db;
    
    auto startTime = std::chrono::system_clock::now();

    for(int i=1; i<argc; ++i)
    {
        if(strcmp(argv[i], "-v")==0)
        {
            verbose = true;
            db.SetVerbose(true);
            continue;
        }
        if(verbose) std::cout << "Reading " << argv[i] << std::endl;

        auto r = db.ReadFile(argv[i]);

        if(r)
        {
            std::cerr << "Failed to read " << argv[i] << std::endl;
            return r;
        }
    }
    
    auto endTime = std::chrono::system_clock::now();
    
    if(verbose)
    {
        std::cout << "Evaluation steps = " << Database::GlobalCallCount() << std::endl;
        std::cout << "Evaluation time  = " << std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count() << " us\n";
    }
    
    
    
    if(db.UserErrorReported())
        return 1;

    return 0;
}
