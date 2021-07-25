#include "Logical.hpp"
#include "antlr4-runtime.h"
#include "JavaLexer.h"
#include "JavaParser.h"

class JavaParser
{
public:
    JavaParser(Logical::Module & module, const char * filename) :
        stream(filename), input(stream), lexer(&input), tokens(&lexer), parser(&tokens),
        ruleNames(parser.getRuleNames()),
        tokenNames(parser.getTokenNames()),
        javafile_filename(module.GetPredicate({"java:file","filename"})),
        javafile_errormessage(module.GetPredicate({"java:file", "errormessage"})),
        javatoken_text(module.GetPredicate({"java:token","text"})),
        javanode_type_parent_index_location(module.GetPredicate({"java:node","type","parent","index","location"})),
        location_filename_startrow_startcol_endrow_endcol(module.GetPredicate({"location","filename","startrow","startcol","endrow","endcol"}))
    {
        auto file = module.NewObject();
        javafile_filename.Set(0, file);
        javafile_filename.Set(1, filename);
        javafile_filename.YieldResult();

        if(stream)
        {
            auto root = parser.compilationUnit();
            if(root)
            {
                walk_tree(root);
                return;
            }
            else
            {
                // TODO: Log more diagnosrtics
                javafile_errormessage.Set(0, file);
                javafile_errormessage.Set(1, "Parse error");
                javafile_errormessage.YieldResult();
            }
        }
        else
        {
            javafile_errormessage.Set(0, file);
            javafile_errormessage.Set(1, "Failed to open file");
            javafile_errormessage.YieldResult();
        }
    }

    void visit_node(antlr4::tree::TerminalNode * r)
    {
        auto text = r->getText();
        auto sym = r->getSymbol();
        auto type = sym->getType();
        auto name = tokenNames[type];
        //std::cout << "Got token " << name << std::endl;
    }

    void walk_tree(antlr4::ParserRuleContext * r)
    {
        auto context = parser.getRuleContext();
        auto src = r->getSourceInterval();
        auto info = r->toInfoString(&parser);
        auto text = r->getText();
        auto index = r->getRuleIndex();
        auto rule = ruleNames[index];
        
        //std::cout << "Got rule " << rule << std::endl;
        for(auto p : r->children)
        {
            if(auto q = dynamic_cast<antlr4::ParserRuleContext *>(p))
                walk_tree((antlr4::ParserRuleContext *)p);
            else if(auto q = dynamic_cast<antlr4::tree::TerminalNode*>(p))
            {
                visit_node(q);
            }
            else
                std::cout << "Unknown node type\n";
        }
    }
    
    std::ifstream stream;
    antlr4::ANTLRInputStream input;
    java::JavaLexer lexer;
    antlr4::CommonTokenStream tokens;
    java::JavaParser parser;
    std::vector<std::string> ruleNames;
    std::vector<std::string> tokenNames;
    
    Logical::Call & javafile_filename;
    Logical::Call & javafile_errormessage;
    Logical::Call & javatoken_text;
    Logical::Call & javanode_type_parent_index_location;
    Logical::Call & location_filename_startrow_startcol_endrow_endcol;
};

static void parsejavafile(Logical::Call & call)
{
    const char * filename;
    if(call.Get(0, filename))
    {
        JavaParser p(call.GetModule(), filename);

    }
    else
    {
        call.GetModule().ReportError("Supplied argument to java:parse is not a string");
    }
}

void RegisterFunctions(Logical::Module & module)
{
    module.AddCommand(parsejavafile, {"java:parse"});
}
