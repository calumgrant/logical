#include <string>
#include <unordered_map>
#include <vector>
#include "persist.h"

typedef std::basic_string<char, std::char_traits<char>, persist::allocator<char>> string_type;

class StringTable
{
public:
    StringTable(persist::shared_memory &mem);

    int GetId(const string_type & value);
    const string_type &GetString(int id) const;
private:
    std::vector<string_type, persist::fast_allocator<string_type>> strings;
    std::unordered_map<string_type, int, std::hash<string_type>, std::equal_to<string_type>, persist::fast_allocator<std::pair<const string_type, int>>> ids;
};
