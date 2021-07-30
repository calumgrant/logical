#pragma once

struct SourceLocation
{
    int filenameId;
    int line, column;
};

SourceLocation operator+(const SourceLocation &l1, const SourceLocation &l2);


