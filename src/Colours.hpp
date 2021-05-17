#pragma once

namespace Colours
{
    static const char * Normal = "\033[0m";
    static const char * Value = "\033[1;35m";  // Light magenta
    static const char * Variable = "\033[0;32m"; // Light green
    static const char * Relation = "\033[0;33m"; // Brown orange
    static const char * Error = "\033[1;31m"; // Light red

    enum class TextHighlight { None, Value, Variable, Relation, Error };
}
