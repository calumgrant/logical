#include <string>
#include <unordered_map>
#include <vector>
#include "Allocator.hpp"

typedef std::basic_string<char, std::char_traits<char>, PERSIST_ALLOCATOR<char>> string_type;

class StringTable
{
public:
    StringTable(AllocatorData &mem);

    int GetId(const string_type & value);
    const string_type &GetString(int id) const;
private:
    std::vector<string_type, PERSIST_ALLOCATOR<string_type>> strings;
    std::unordered_map<string_type, int, std::hash<string_type>, std::equal_to<string_type>, PERSIST_ALLOCATOR<std::pair<const string_type, int>>> ids;
};
