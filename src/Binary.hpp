#pragma once

#include <numeric>

class Entity;

void WriteBinaryEntity(const Entity &e, std::ostreambuf_iterator<char> & output);
void WriteBinaryEntity(const Entity &e, std::ostream_iterator<char> & output);
void WriteBinaryEntity(const Entity &e, char * & output, char * end);
Entity ReadBinaryEntity(std::istreambuf_iterator<char> & input);
Entity ReadBinaryEntity(std::istream_iterator<char> & input);
Entity ReadBinaryEntity(char * & input, char * end);

