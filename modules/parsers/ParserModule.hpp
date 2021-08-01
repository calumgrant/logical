#pragma once
#include <Logical.hpp>
#include <memory>
#include <vector>

#include "Language.hpp"

class ParserModule
{
public:
    void AddLanguage(std::unique_ptr<Language> && language);
    void Parse(Logical::Call & call, const char * file, const char * language = nullptr) const;
private:
    void ParseFile(Logical::Call & call, const char * file, const char * language) const;
    std::vector<std::unique_ptr<Language>> languages;
};
