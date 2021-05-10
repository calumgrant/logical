#include <iostream>
#include "Database.hpp"
#include "tokens.tab.h"

// extern "C" FILE * yyin;

void yyrestart (FILE *input_file ,yyscan_t yyscanner );
int yylex_init (yyscan_t* scanner);
int yylex_init_extra (Database *, yyscan_t* scanner);
int yylex_destroy (yyscan_t yyscanner );
void yyset_in  (FILE * in_str ,yyscan_t yyscanner );

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
            yyscan_t scanner;

//             yylex_init(&scanner);
            yylex_init_extra(&db, &scanner);

            yyset_in(f, scanner);
            // yyin = f;
            yyrestart(f, scanner);
            int p = yyparse(scanner, db);
            //if(p==0) std::cout << "Parse success!\n";
            //else std::cerr << "Failed to parse " << argv[i] << std::endl;
            fclose(f);
            if(p) return 128;
            yylex_destroy(scanner);
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
