#include <iostream>
#include "Database.hpp"

extern "C" FILE * yyin;
int yyparse(Database &db);
int yyrestart(FILE*);

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
        FILE * f = fopen(argv[i], "r");

        if(f)
        {
            yyin = f;
            yyrestart(yyin);
            int p = yyparse(db);
            //if(p==0) std::cout << "Parse success!\n";
            //else std::cerr << "Failed to parse " << argv[i] << std::endl;
            fclose(f);
            if(p) return 128;
        }
        else
        {
            std::cerr << "Could not open " << argv[i] << std::endl;
            return 128;
        }
    }
    
    if(db.UserErrorReported())
        return 1;

    return 0;
}
