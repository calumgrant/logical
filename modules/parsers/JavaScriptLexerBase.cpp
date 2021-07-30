#include "JavaScriptLexer.h"

using namespace antlr4;
using namespace javascript;

JavaScriptLexerBase::JavaScriptLexerBase(CharStream *input) : Lexer(input)
{
}

bool JavaScriptLexerBase::getStrictDefault()
{
    return useStrictDefault;
}

bool JavaScriptLexerBase::IsStartOfFile(){
    // No token has been produced yet: at the start of the input,
    // no division is possible, so a regex literal _is_ possible.
    return lastToken;
}

void JavaScriptLexerBase::setUseStrictDefault(bool value)
{
    useStrictDefault = value;
    useStrictCurrent = value;
}

bool JavaScriptLexerBase::IsStrictMode()
{
    return useStrictCurrent;
}

std::unique_ptr<antlr4::Token> JavaScriptLexerBase::nextToken() {
    auto next = Lexer::nextToken();

    if (next->getChannel() == Token::DEFAULT_CHANNEL) {
        // Keep track of the last token on the default channel.
        lastToken = true;
        lastTokenType = next->getType();
    }

    return next;
}

void JavaScriptLexerBase::ProcessOpenBrace()
{
    useStrictCurrent = scopeStrictModes.size() > 0 && scopeStrictModes.top() ? true : useStrictDefault;
    scopeStrictModes.push(useStrictCurrent);
}

void JavaScriptLexerBase::ProcessCloseBrace()
{
    if (scopeStrictModes.size() > 0) {
        useStrictCurrent = scopeStrictModes.top();
        scopeStrictModes.pop();
    } else {
        useStrictCurrent = useStrictDefault;
    }
}

void JavaScriptLexerBase::ProcessStringLiteral()
{
    if (lastToken || lastTokenType == JavaScriptLexer::OpenBrace)
    {
        std::string text = getText();
        if (text == "\"use strict\"" || text == "'use strict'")
        {
            if (scopeStrictModes.size() > 0)
                scopeStrictModes.pop();
            useStrictCurrent = true;
            scopeStrictModes.push(useStrictCurrent);
        }
    }
}

bool JavaScriptLexerBase::IsRegexPossible()
{
    if (lastToken) {
        // No token has been produced yet: at the start of the input,
        // no division is possible, so a regex literal _is_ possible.
        return true;
    }
    
    switch (lastTokenType) {
        case JavaScriptLexer::Identifier:
        case JavaScriptLexer::NullLiteral:
        case JavaScriptLexer::BooleanLiteral:
        case JavaScriptLexer::This:
        case JavaScriptLexer::CloseBracket:
        case JavaScriptLexer::CloseParen:
        case JavaScriptLexer::OctalIntegerLiteral:
        case JavaScriptLexer::DecimalLiteral:
        case JavaScriptLexer::HexIntegerLiteral:
        case JavaScriptLexer::StringLiteral:
        case JavaScriptLexer::PlusPlus:
        case JavaScriptLexer::MinusMinus:
            // After any of the tokens above, no regex literal can follow.
            return false;
        default:
            // In all other cases, a regex literal _is_ possible.
            return true;
    }
}

void JavaScriptLexerBase::IncreaseTemplateDepth()
{
    ++templateDepth;
}

void JavaScriptLexerBase::DecreaseTemplateDepth()
{
    --templateDepth;
}

bool JavaScriptLexerBase::IsInTemplateString() const
{
    return templateDepth>0;
}