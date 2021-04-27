#include <iostream>
#include "Database.hpp"

extern "C" FILE * yyin;
int yyparse(Database &db);

int main(int argc, char**argv)
{
    if(argc==1)
    {
        std::cout << "Usage: logical <filename> ...\n";
        return 127;
    }

    Database db;

    for(int i=1; i<argc; ++i)
    {
        FILE * f = fopen(argv[i], "r");

        if(f)
        {
            yyin = f;
            int p = yyparse(db);
            if(p==0) std::cout << "Parse success!\n";
            fclose(f);
            if(!p) return 128;
        }
        else
        {
            std::cerr << "Could not open " << argv[i] << std::endl;
            return 128;
        }
    }
}
