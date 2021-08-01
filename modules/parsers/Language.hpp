#pragma once
#include <Logical.hpp>
#include "Parser.hpp"
#include "Predicates.hpp"
#include <antlr4-runtime.h>
#include <filesystem>

class Language
{
public:
    virtual ~Language();
    virtual void ParseFile(const char * filename, Logical::Module & module) const =0;
    virtual bool CanParse(const std::filesystem::path & p, const char * language) const =0;
    virtual const char * Type() const =0;
};

template<typename AntlrLexer, typename AntlrParser, typename Rc, Rc * (AntlrParser::*ParseFn)()>
class AntlrLanguage : public Language
{
public:
    AntlrLanguage(const char * name, const char * extension) : name(name), extension(extension)
    {
    }

    virtual void ParseFile(const char * filename, Logical::Module & module) const override
    {
        Predicates predicates(module);
        ::Parser<AntlrLexer, AntlrParser, Rc, ParseFn> parser(module, filename, predicates, name.c_str());
    }
    
    const char * Type() const override { return name.c_str(); }

    bool CanParse(const std::filesystem::path & p, const char * language) const override
    {
        return p.extension() == extension && (!language || name == language);
    }
    
private:
    std::string name, extension;
};
