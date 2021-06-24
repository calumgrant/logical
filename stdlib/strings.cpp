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

static const char * messageText[] = { "Promogulating",
    "Spawning", "Refining", "Reifying", "Injecting", "Pronouncing", "Ephemerating", "Proscribing", "Transliterating", "Emptying the bath", "Watching the paint dry", "Cleaning the patio", "Sweeping under the rug", "Asserting assertions", "Retracting", "Stating facts", "Stating the obvious", "Proliferating", "Expunging", "Counting tuples", "AcquiAcquiring locksringlocks", "Cleaning the toilet", "Collecting garbage", "Effusing results", "Starting cloud service", "Bootstrapping bootstraps", "Boosting availability", "Boosting productivity", "Counting cores", "Starting botnet", "Laying plans", "Setting traps", "Pontificating", "Respawning", "Prevaricating", "Navel-gazing", "Counting sheep", "Backtracking", "Starting infinite loop", "Increasing entropy", "Commutating commutators", "Closing transitively", "Assuming a closed world", "Enjoying the weather", "Raising the dead", "Invoking dark magic", "Summoning Bal'thazaar", "Trying to remember De Morgan", "Being optimistic", "Opening portal", "Untangling spaghetti", "Staring at feet", "Finding fixed-points", "Out to lunch", "Chasing wild geese", "Iterating", "Recursing", "Cursing again", "Trying on shoes", "Herding cats", "Praying", "Begging for mercy", "Crossing fingers", "Touching wood", "Allocating memory", "Cursing Bill Gates", "Raising expectations", "Weighing alternatives", "Reordering joins", "Going on a diet",
    
    nullptr };

void messages(Call & call)
{
    for(auto m = messageText; *m; ++m)
    {
        call.Set(0, *m);
        call.YieldResult();
    }
}

void messageoftheday(Call & call)
{
    const auto count = sizeof(messageText)/sizeof(messageText[0]);
    // int n =
}


