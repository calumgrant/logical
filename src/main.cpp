#include <iostream>
#include "Database.hpp"

int main(int argc, char**argv)
{
    if(argc==1)
    {
        std::cout << "Usage: logical <filename> ...\n";
        return 127;
    }

    int verbose=0;
    Database db;

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
    
    if(verbose)
        std::cout << "Total evaluation steps = " << Database::GlobalCallCount() << std::endl;
    
    if(db.UserErrorReported())
        return 1;

    return 0;
}
