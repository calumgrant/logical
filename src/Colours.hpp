#pragma once

namespace Colours
{
    extern const char * Normal;
    extern const char * Value;
    extern const char * Variable;
    extern const char * IntroducedVariable;
    extern const char * Success;
    extern const char * Relation;
    extern const char * Error;
    extern const char * Detail;

    enum class TextHighlight { None, Value, Variable, Relation, Error };
}
