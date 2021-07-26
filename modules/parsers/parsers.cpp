#include "Logical.hpp"
#include "antlr4-runtime.h"
#include "JavaLexer.h"
#include "JavaParser.h"

#include <filesystem>

class JavaParser
{
public:
    JavaParser(Logical::Module & module, const char * filename) :
        module(module),
        filename(filename),
        stream(filename),
        input(stream),
        lexer(&input),
        tokens(&lexer),
        parser(&tokens),
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

        lexer.removeErrorListeners();
        parser.removeErrorListeners();

        if(stream)
        {
            auto root = parser.compilationUnit();
            if(root)
            {
                walk_tree(file, 0, root);
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

    void visit_node(Logical::Entity parent, int childIndex, antlr4::tree::TerminalNode * r)
    {
        auto text = r->getText();
        auto sym = r->getSymbol();
        auto type = sym->getType();
        auto name = tokenNames[type];
        //std::cout << "Got token " << name << std::endl;
        auto line = sym->getLine();
        auto col = sym->getCharPositionInLine();
        auto len = sym->getStopIndex()-sym->getStartIndex();

        // Set this to 0 to benchmark just the parsing performance
#define ENABLE_STORE 1
        
#if ENABLE_STORE
        auto node = module.NewObject();
        location_filename_startrow_startcol_endrow_endcol.Set(0, node);
        location_filename_startrow_startcol_endrow_endcol.Set(1, filename);
        location_filename_startrow_startcol_endrow_endcol.Set(2, (Logical::Int)line);
        location_filename_startrow_startcol_endrow_endcol.Set(3, (Logical::Int)col);
        location_filename_startrow_startcol_endrow_endcol.Set(4, (Logical::Int)line);
        location_filename_startrow_startcol_endrow_endcol.Set(5, (Logical::Int)(col+text.size()));
        location_filename_startrow_startcol_endrow_endcol.YieldResult();
        
        javanode_type_parent_index_location.Set(0, node);
        javanode_type_parent_index_location.Set(1, name.c_str());
        javanode_type_parent_index_location.Set(2, parent);
        javanode_type_parent_index_location.Set(3, (Logical::Int)childIndex);
        javanode_type_parent_index_location.Set(4, node);
        javanode_type_parent_index_location.YieldResult();
        
        javatoken_text.Set(0, node);
        javatoken_text.Set(1, r->getText().c_str());
        javatoken_text.YieldResult();
#endif
    }

    void walk_tree(Logical::Entity parent, int childIndex, antlr4::ParserRuleContext * r)
    {
        auto context = parser.getRuleContext();
        auto src = r->getSourceInterval();
        // auto info = r->toInfoString(&parser);
        // auto text = r->getText();
        auto ruleIndex = r->getRuleIndex();
        auto rule = ruleNames[ruleIndex];
        auto start = r->getStart();
        auto stop = r->getStop();

#if ENABLE_STORE
        
        auto node = module.NewObject();
        
        location_filename_startrow_startcol_endrow_endcol.Set(0, node);
        location_filename_startrow_startcol_endrow_endcol.Set(1, filename);
        location_filename_startrow_startcol_endrow_endcol.Set(2, (Logical::Int)start->getLine());
        location_filename_startrow_startcol_endrow_endcol.Set(3, (Logical::Int)start->getCharPositionInLine());
        location_filename_startrow_startcol_endrow_endcol.Set(4, (Logical::Int)stop->getLine());
        location_filename_startrow_startcol_endrow_endcol.Set(5, (Logical::Int)stop->getCharPositionInLine());
        location_filename_startrow_startcol_endrow_endcol.YieldResult();
        
        javanode_type_parent_index_location.Set(0, node);
        javanode_type_parent_index_location.Set(1, rule.c_str());
        javanode_type_parent_index_location.Set(2, parent);
        javanode_type_parent_index_location.Set(3, (Logical::Int)childIndex);
        javanode_type_parent_index_location.Set(4, node);
        javanode_type_parent_index_location.YieldResult();
#else
        Logical::Entity node;
#endif
        
        //std::cout << "Got rule " << rule << std::endl;
        int i=0;
        for(auto p : r->children)
        {
            if(auto q = dynamic_cast<antlr4::ParserRuleContext *>(p))
                walk_tree(node, i, (antlr4::ParserRuleContext *)p);
            else if(auto q = dynamic_cast<antlr4::tree::TerminalNode*>(p))
            {
                visit_node(node, i, q);
            }
            else
                std::cout << "Unknown node type\n";
            ++i;
        }
    }
    
    Logical::Module & module;
    const char * filename;
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

void WalkDirectory(Logical::Module & module, const char * path)
{
    std::filesystem::path p(path);
    std::vector<std::filesystem::path> javafiles;
    std::size_t size=0;
    
    if(std::filesystem::is_directory(path))
    {
        for(auto it = std::filesystem::recursive_directory_iterator(p); it != std::filesystem::recursive_directory_iterator(); ++it)
        {
            auto p = it->path();
            if(it->is_regular_file() && p.extension() == ".java")
            {
                javafiles.push_back(p);
                size += it->file_size();
            }
        }
    }
    
    std::cout << "Found " << javafiles.size() << " Java files in " << path << ", " << size << " bytes total\n";

    int count=0;
    const int step = 100;
    std::cout << "[";
    for(int i=0; i<javafiles.size(); i+=step)
        std::cout << "-";
    std::cout << "]\n[";
    for(auto & j : javafiles)
    {
        //std::cout << j << std::endl;
        if(count%step == 0)
        {
            std::cout << ">" << std::flush;
        }
        ++count;
        try
        {
            JavaParser(module, j.c_str());
        }
        catch(std::bad_alloc&)
        {
            module.ReportError("Memory limit exceeded");
            throw;
        }
        catch(std::exception &ex)
        {
            module.ReportError(ex.what());
        }
        catch(...)
        {
            module.ReportError("Uncaught exception when parsing");
        }
    }
    std::cout << "]\n";
}

static void parsejavadirectory(Logical::Call & call)
{
    const char * pathname;
    if(call.Get(0, pathname))
    {
        WalkDirectory(call.GetModule(), pathname);
    }
    else
    {
        call.GetModule().ReportError("Supplied argument to java:parse is not a string");
    }

}

void RegisterFunctions(Logical::Module & module)
{
    module.AddCommand(parsejavafile, {"java:parse"});
    module.AddCommand(parsejavadirectory, {"java:parse-directory"});
}
