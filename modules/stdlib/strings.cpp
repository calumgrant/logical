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
    call.Error(ss.str().c_str());
}

void errorVarargs(Call & call)
{
    call.Error("");
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

template<typename It>
class CsvParser
{
public:
    int column = 0;
    CsvParser(It start, It end) : current(start), end(end) {}
    It current, end;
    It tok_start, tok_end;
    bool emptyEnd = false;
    
    enum class CsvParserState
    {
        Begin,
        InContent,
        InQuote,
        AfterQuote,
        BackslashInQuote,
        BackslashOutsideQuote
    };

    bool Next()
    {
        ++column;
        auto state = CsvParserState::Begin;
        
        for(; current!=end; ++current)
        {
            switch(state)
            {
                case CsvParserState::Begin:
                    switch(*current)
                    {
                        case ',':
                            tok_start = tok_end = current;
                            ++current;
                            emptyEnd = true;
                            return true;
                        case ' ':
                        case '\t':
                            break;
                        case '\"':
                            tok_end = tok_start = current+1;
                            state = CsvParserState::InQuote;
                            break;
                        default:
                            tok_start = current;
                            tok_end = tok_start+1;
                            state = CsvParserState::InContent;
                            break;
                    }
                    break;
                case CsvParserState::InContent:
                    switch(*current)
                    {
                        case ',':
                            ++current;
                            emptyEnd = true;
                            return true;
                        case '\\':
                            state = CsvParserState::BackslashOutsideQuote;
                            break;
                        case ' ':
                        case '\t':
                            break;
                        default:
                            tok_end = current+1;
                            break;
                    }
                    break;
                case CsvParserState::InQuote:
                    switch(*current)
                    {
                        case '\\':
                            state = CsvParserState::BackslashOutsideQuote;
                            break;
                        case '\"':
                            tok_end = current;
                            state = CsvParserState::AfterQuote;
                            break;
                    }
                    break;
                case CsvParserState::AfterQuote:
                    switch(*current)
                    {
                        case ',':
                            ++current;
                            return true;
                    }
                    break;
                case CsvParserState::BackslashInQuote:
                    state = CsvParserState::InQuote;
                    break;
                case CsvParserState::BackslashOutsideQuote:
                    state = CsvParserState::InContent;
                    break;
            }
        }
        
        // End handling todo
        switch(state)
        {
            case CsvParserState::AfterQuote:
            case CsvParserState::InContent:
                emptyEnd = false;
                return true;
                
            case CsvParserState::Begin:
                if(!emptyEnd) return false;
                emptyEnd = false;
                tok_start = tok_end = current;
                return true;
            default:
                return false;
        }
    }
};

void readcsv(Call & call)
{
    const char * filename;
    if(call.Get(0, filename))
    {
        Call & contents = call.GetModule().GetPredicate({"csv:file", "row", "col", "text"});
        contents.Set(0, filename);
        std::ifstream file(filename);
        std::string line;
        Int row=1;
        while(std::getline(file, line))
        {
            contents.Set(1, row++);
            CsvParser<std::string::iterator> parser(line.begin(), line.end());

            while(parser.Next())
            {
                contents.Set(2, (Int)parser.column);
                auto debug = parser.tok_end - parser.tok_start;
                if(parser.tok_end != line.end()) *parser.tok_end = 0;
                contents.Set(3, &*parser.tok_start);
                contents.YieldResult();
            }
        }
    }
}

void stringcharacterpositionBFF(Call & call)
{
    const char * str;
    if(call.Get(0, str))
    {
        auto len = std::strlen(str);

        for(Int i=0; i<len; ++i)
        {
            char ch[2] = { str[i], 0 };
            call.Set(1, ch);
            call.Set(2, i);
            call.YieldResult();
        }
    }
}

void stringcharacterpositionBBB(Call & call)
{
    const char * str, * ch;
    Int position;
    if(call.Get(0, str) && call.Get(1, ch) && call.Get(2, position))
    {
        auto len = std::strlen(str);
        auto chlen = std::strlen(ch);

        if(position>=0 && position < len && chlen == 1 && str[position] == ch[0])
            call.YieldResult();
    }
}

void stringcharacterpositionBBF(Call & call)
{
    const char * str, * ch;
    if(call.Get(0, str) && call.Get(1, ch))
    {
        auto strL = std::strlen(str);
        auto chL = std::strlen(ch);
        if(chL==1)
        {
            for(Int position = 0; position<strL; ++position)
            {
                if(ch[0] == str[position])
                {
                    call.Set(2, position);
                    call.YieldResult();
                }
            }
        }
    }
}

void stringcharacterpositionBFB(Call & call)
{
    const char * str;
    Int position;
    if(call.Get(0, str) && call.Get(2, position))
    {
        auto len = std::strlen(str);
        if(position>=0 && position < len)
        {
            char ch[2] = { str[position], 0 };
            call.Set(1, ch);
            call.YieldResult();
        }
    }
}
