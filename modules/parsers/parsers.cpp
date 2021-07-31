#include "Logical.hpp"
#include "antlr4-runtime.h"
#include "JavaLexer.h"
#include "JavaParser.h"
#include "JavaScriptLexer.h"
#include "JavaScriptParser.h"

#include <filesystem>

struct Predicates
{
    Predicates(Logical::Module & module) :
        javafile_filename(module.GetPredicate({"java:file","filename"})),
        javafile_errormessage(module.GetPredicate({"java:file", "errormessage"})),
        javatoken_text(module.GetPredicate({"java:token","text"})),
        javanode_type_parent_index_location(module.GetPredicate({"java:node","type","parent","index","location"})),
        location_filename_startrow_startcol_endrow_endcol(module.GetPredicate({"location","filename","startrow","startcol","endrow","endcol"}))
    {
    }
    
    void Finalize()
    {
        javafile_filename.Finalize();
        javafile_errormessage.Finalize();
        javatoken_text.Finalize();
        javanode_type_parent_index_location.Finalize();
        javanode_type_parent_index_location.Finalize();
        location_filename_startrow_startcol_endrow_endcol.Finalize();
    }

    Logical::Call & javafile_filename;
    Logical::Call & javafile_errormessage;
    Logical::Call & javatoken_text;
    Logical::Call & javanode_type_parent_index_location;
    Logical::Call & location_filename_startrow_startcol_endrow_endcol;
};


template<typename AntlrLexer, typename AntlrParser>
class Parser
{
public:
    Parser(Logical::Module & module, const char * filename, Predicates & predicates) :
        module(module),
        filename(filename),
        stream(filename),
        input(stream),
        lexer(&input),
        tokens(&lexer),
        parser(&tokens),
        ruleNames(parser.getRuleNames()),
        tokenNames(parser.getTokenNames()),
        predicates(predicates)
    {
        auto file = module.NewObject();
        predicates.javafile_filename.Set(0, file);
        predicates.javafile_filename.Set(1, filename);
        predicates.javafile_filename.YieldResult();

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
                predicates.javafile_errormessage.Set(0, file);
                predicates.javafile_errormessage.Set(1, "Parse error");
                predicates.javafile_errormessage.YieldResult();
            }
        }
        else
        {
            predicates.javafile_errormessage.Set(0, file);
            predicates.javafile_errormessage.Set(1, "Failed to open file");
            predicates.javafile_errormessage.YieldResult();
        }
    }

    void visit_node(Logical::Entity parent, int childIndex, antlr4::tree::TerminalNode * r)
    {
        auto text = r->getText();
        auto sym = r->getSymbol();
        auto type = sym->getType();
        auto name = type<tokenNames.size() ? tokenNames.at(type).c_str() : "";
        
        //std::cout << "Got token " << name << std::endl;
        auto line = sym->getLine();
        auto col = sym->getCharPositionInLine();
        auto len = sym->getStopIndex()-sym->getStartIndex();

        // Set this to 0 to benchmark just the parsing performance
#define ENABLE_STORE 1
        
#if ENABLE_STORE
        auto node = module.NewObject();
        predicates.location_filename_startrow_startcol_endrow_endcol.Set(0, node);
        predicates.location_filename_startrow_startcol_endrow_endcol.Set(1, filename);
        predicates.location_filename_startrow_startcol_endrow_endcol.Set(2, (Logical::Int)line);
        predicates.location_filename_startrow_startcol_endrow_endcol.Set(3, (Logical::Int)col);
        predicates.location_filename_startrow_startcol_endrow_endcol.Set(4, (Logical::Int)line);
        predicates.location_filename_startrow_startcol_endrow_endcol.Set(5, (Logical::Int)(col+text.size()));
        predicates.location_filename_startrow_startcol_endrow_endcol.YieldResult();
        
        predicates.javanode_type_parent_index_location.Set(0, node);
        predicates.javanode_type_parent_index_location.Set(1, name);
        predicates.javanode_type_parent_index_location.Set(2, parent);
        predicates.javanode_type_parent_index_location.Set(3, (Logical::Int)childIndex);
        predicates.javanode_type_parent_index_location.Set(4, node);
        predicates.javanode_type_parent_index_location.YieldResult();
        
        predicates.javatoken_text.Set(0, node);
        predicates.javatoken_text.Set(1, r->getText().c_str());
        predicates.javatoken_text.YieldResult();
#endif
    }

    void walk_tree(Logical::Entity parent, int childIndex, antlr4::ParserRuleContext * r)
    {
        auto context = parser.getRuleContext();
        auto src = r->getSourceInterval();
        // auto info = r->toInfoString(&parser);
        // auto text = r->getText();
        auto ruleIndex = r->getRuleIndex();
        auto rule = ruleNames.at(ruleIndex).c_str();
        auto start = r->getStart();
        auto stop = r->getStop();

#if ENABLE_STORE
        
        auto node = module.NewObject();
        
        predicates.location_filename_startrow_startcol_endrow_endcol.Set(0, node);
        predicates.location_filename_startrow_startcol_endrow_endcol.Set(1, filename);
        predicates.location_filename_startrow_startcol_endrow_endcol.Set(2, (Logical::Int)start->getLine());
        predicates.location_filename_startrow_startcol_endrow_endcol.Set(3, (Logical::Int)start->getCharPositionInLine());
        predicates.location_filename_startrow_startcol_endrow_endcol.Set(4, (Logical::Int)stop->getLine());
        predicates.location_filename_startrow_startcol_endrow_endcol.Set(5, (Logical::Int)stop->getCharPositionInLine());
        predicates.location_filename_startrow_startcol_endrow_endcol.YieldResult();
        
        predicates.javanode_type_parent_index_location.Set(0, node);
        predicates.javanode_type_parent_index_location.Set(1, rule);
        predicates.javanode_type_parent_index_location.Set(2, parent);
        predicates.javanode_type_parent_index_location.Set(3, (Logical::Int)childIndex);
        predicates.javanode_type_parent_index_location.Set(4, node);
        predicates.javanode_type_parent_index_location.YieldResult();
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
    
    Predicates & predicates;
};

typedef Parser<java::JavaLexer, java::JavaParser> JavaParser;
typedef Parser<javascript::JavaScriptLexer, javascript::JavaScriptParser> JavaScriptParser;

class Language
{
public:
    virtual ~Language();
    virtual void ParseFile(const char * filename, Logical::Module & module) const =0;
    virtual bool CanParse(const std::filesystem::path & p) const =0;
    virtual const char * Type() const =0;
};

template<typename Lexer, typename Parser>
class AntlrLanguage : public Language
{
public:
    AntlrLanguage(const char * name, const char * extension) : name(name), extension(extension)
    {
    }

    virtual void ParseFile(const char * filename, Logical::Module & module) const override
    {
        Predicates predicates(module);
        ::Parser<Lexer, Parser> parser(module, filename, predicates);
    }
    
    const char * Type() const override { return name.c_str(); }

    bool CanParse(const std::filesystem::path & p) const override
    {
        return p.extension() == extension;
    }
    
private:
    std::string name, extension;
};

// !! ParserModule.hpp
class ParserModule
{
public:
    void AddLanguage(std::unique_ptr<Language> && language);
    void Parse(Logical::Call & call, const char * file, const char * language = nullptr) const;
private:
    void ParseFile(Logical::Call & call, const char * file, const char * language) const;
    std::vector<std::unique_ptr<Language>> languages;
};

Language::~Language() {}

void ParserModule::AddLanguage(std::unique_ptr<Language> && language)
{
    languages.push_back(std::move(language));
}

void ParserModule::Parse(Logical::Call & call, const char * filename, const char * language) const
{
    std::filesystem::path p(filename);
    if(std::filesystem::is_regular_file(p))
    {
        ParseFile(call, filename, language);
        // Check the type
    }
    else if(std::filesystem::is_directory(p))
    {
        std::vector<std::filesystem::path> filesToParse;
        std::size_t size=0;

        for(auto it = std::filesystem::recursive_directory_iterator(p); it != std::filesystem::recursive_directory_iterator(); ++it)
        {
            auto p = it->path();
            for(auto & lang : languages)
            {
                if(lang->CanParse(*it))
                {
                    filesToParse.push_back(p);
                    size += it->file_size();
                    break;
                }
            }
        }
    
        std::cout << "Parsing " << filesToParse.size() << " files in " << p << ", " << size << " bytes total\n";

        int count=0;
        int step = filesToParse.size()/100;
        if(step<1) step=1;
        std::cout << "[";
        for(int i=0; i<filesToParse.size(); i+=step)
            std::cout << "-";
        std::cout << "]\n[";
        
        for(auto & j : filesToParse)
        {
            //std::cout << j << std::endl;
            if(count%step == 0)
            {
                std::cout << ">" << std::flush;
            }
            ++count;
            try
            {
                for(auto & lang : languages)
                {
                    if(lang->CanParse(j))
                        lang->ParseFile(j.c_str(), call.GetModule());
                }
            }
            catch(std::bad_alloc&)
            {
                call.Error("Memory limit exceeded");
                throw;
            }
            catch(std::exception &ex)
            {
                call.Error(ex.what());
            }
            catch(...)
            {
                call.Error("Uncaught exception when parsing");
                throw;
            }
        }
        // predicates.Finalize();
        std::cout << "]\n";
    }
}

void ParserModule::ParseFile(Logical::Call & call, const char * filename, const char * language) const
{
    for(auto & lang : languages)
    {
        if(lang->CanParse(filename))
            lang->ParseFile(filename, call.GetModule());
    }
}

static ParserModule parserModule;

static void parse(Logical::Call & call)
{
    const char * path;
    if(call.Get(0, path))
    {
        auto pm = (ParserModule*)call.GetData();
        pm->Parse(call, path);
    }
    else
    {
        call.Error("The supplied argument was not a string");
    }
}

static void parse_language(Logical::Call & call)
{
    const char *path, *language;
    if(call.Get(0, path) && call.Get(1, language))
    {
        auto pm = (ParserModule*)call.GetData();
        pm->Parse(call, path, language);
    }
    else
    {
        call.Error("The supplied argument was not a string");
    }
}

void RegisterFunctions(Logical::Module & module)
{
    parserModule.AddLanguage(std::make_unique<AntlrLanguage<java::JavaLexer, java::JavaParser>>("Java", ".java"));
    parserModule.AddLanguage(std::make_unique<AntlrLanguage<javascript::JavaScriptLexer, javascript::JavaScriptParser>>("JavaScript", ".js"));
    
    module.AddCommand(parse_language, {"parsers:parse", "language"}, &parserModule);
    module.AddCommand(parse, {"parsers:parse"}, &parserModule);
}
