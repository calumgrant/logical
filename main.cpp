#include <iostream>

extern "C" FILE * yyin;
// extern "C" 
int yylex();
int yyparse();
// const char * yytext();
// const char * YYTEXT;
extern char yytext[];

int main(int argc, char**argv)
{
    if(argc==1)
    {
        std::cout << "Usage: logical <filename> ...\n";
        return 127;
    }

    for(int i=1; i<argc; ++i)
    {
        FILE * f = fopen(argv[i], "r");

        if(f)
        {
            yyin = f;
            int p = yyparse();
            if(p==0) std::cout << "Parse success\n";
            while(int c = yylex())
            {
                std::cout << c << "=[" << yytext << "]";
//                std::cout << c << " " << yytext << std::endl;
            }
            fclose(f);

        }
        else
        {
            std::cerr << "Could not open " << argv[i] << std::endl;
            return 128;
        }
    }
}
