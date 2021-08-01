#pragma once
#include "Logical.hpp"
#include "Predicates.hpp"
#include <antlr4-runtime.h>

template<typename AntlrLexer, typename AntlrParser, typename Rc, Rc * (AntlrParser::*ParseFn)()>
class Parser
{
public:
    Parser(Logical::Module & module, const char * filename, Predicates & predicates, const char * language) :
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
        predicates.file_filename_language.Set(0, file);
        predicates.file_filename_language.Set(1, filename);
        predicates.file_filename_language.Set(2, language);
        predicates.file_filename_language.YieldResult();
        
        predicates.node_type_parent_index_location_language.Set(5, language);

        lexer.removeErrorListeners();
        parser.removeErrorListeners();

        if(stream)
        {
            auto root = std::invoke(ParseFn, parser);
            if(root)
            {
                walk_tree(file, 0, root);
                return;
            }
            else
            {
                // TODO: Log more diagnosrtics
                predicates.file_errormessage.Set(0, file);
                predicates.file_errormessage.Set(1, "Parse error");
                predicates.file_errormessage.YieldResult();
            }
        }
        else
        {
            predicates.file_errormessage.Set(0, file);
            predicates.file_errormessage.Set(1, "Failed to open file");
            predicates.file_errormessage.YieldResult();
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
        
        predicates.node_type_parent_index_location_language.Set(0, node);
        predicates.node_type_parent_index_location_language.Set(1, name);
        predicates.node_type_parent_index_location_language.Set(2, parent);
        predicates.node_type_parent_index_location_language.Set(3, (Logical::Int)childIndex);
        predicates.node_type_parent_index_location_language.Set(4, node);
        predicates.node_type_parent_index_location_language.YieldResult();
        
        predicates.token_text.Set(0, node);
        predicates.token_text.Set(1, r->getText().c_str());
        predicates.token_text.YieldResult();
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
        
        predicates.node_type_parent_index_location_language.Set(0, node);
        predicates.node_type_parent_index_location_language.Set(1, rule);
        predicates.node_type_parent_index_location_language.Set(2, parent);
        predicates.node_type_parent_index_location_language.Set(3, (Logical::Int)childIndex);
        predicates.node_type_parent_index_location_language.Set(4, node);
        predicates.node_type_parent_index_location_language.YieldResult();
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
    AntlrLexer lexer;
    antlr4::CommonTokenStream tokens;
    AntlrParser parser;
    std::vector<std::string> ruleNames;
    std::vector<std::string> tokenNames;
    
    Predicates & predicates;
};
