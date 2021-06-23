#include "functions.hpp"

#include <cstring>
#include <cctype>
#include <iostream>
#include <sstream>
#include <fstream>
#include <regex>

static void print(Call &call, std::ostream & os)
{
    const char * str;
    double d;
    Int i;
    bool b;
    
    if(call.Get(0, str))
        os << str;
    else if(call.Get(0, d))
        os << d;
    else if(call.Get(0, i))
        os << i;
    else if(call.Get(0, b))
        os << (b?"true":"false");
    else if(call.GetAtString(0, str))
        os << "@" << str;
    else
        os << "?";
}

void print(Call & call)
{
    print(call, std::cout);
    std::cout << std::endl;
}

void error(Call & call)
{
    std::stringstream ss;
    print(call, ss);
    call.GetModule().ReportError(ss.str().c_str());
}

void strlen(Call & call)
{
    const char * str;
    if(call.Get(0, str))
    {
        call.Set(1, (Int)std::strlen(str));
        call.YieldResult();
    }
}

void lowercase(Call & call)
{
    const char * str;
    if(call.Get(0, str))
    {
        auto len = std::strlen(str);
        char tmp[len+1];
        for(int i=0; i<len; ++i)
            tmp[i] = std::tolower(str[i]);
        tmp[len]=0;
        call.Set(1, tmp);
        call.YieldResult();
    }
}

void uppercase(Call & call)
{
    const char * str;
    if(call.Get(0, str))
    {
        auto len = std::strlen(str);
        char tmp[len+1];
        for(int i=0; i<len; ++i)
            tmp[i] = std::toupper(str[i]);
        tmp[len]=0;
        call.Set(1, tmp);
        call.YieldResult();
    }
}

void readcontents(Call & call)
{
    const char * name;
    if(call.Get(0, name))
    {
        std::ifstream file(name);
        std::stringstream ss;
        ss << file.rdbuf();
        call.Set(1, ss.str().c_str());
        call.YieldResult();
    }
}

void readlines(Call & call)
{
    const char * name;
    if(call.Get(0, name))
    {
        std::ifstream file(name);
        for(Int line = 1; file; ++line)
        {
            std::string str;
            std::getline(file, str);

            if(file)
            {
                call.Set(1, line);
                call.Set(2, str.c_str());
                call.YieldResult();
            }
        }
    }
}

void writelines(Call & call)
{
    
}

void writecontents(Call & call)
{
    const char * name;
    const char * contents;
    if(call.Get(0, name) && call.Get(1, contents))
    {
        std::ofstream file(name);
        file << contents;
    }
}

void regexmatch(Call & call)
{
    const char * str, *regex;
    if(call.Get(1, str) && call.Get(0, regex))
    {
        std::regex re(regex);
        if (std::regex_match(str, re))
            call.YieldResult();
    }
}

void regexmatchgroup(Call & call)
{
    const char * str, *regex;
    if(call.Get(1, str) && call.Get(0, regex))
    {
        std::regex re(regex);
        std::match_results<const char*> match;
        if (std::regex_match(str, match, re))
        {
            Int index=0;
            for(auto & m : match)
            {
                call.Set(2, index++);
                call.Set(3, m.str().c_str());
                call.YieldResult();
            }
        }
    }
}
