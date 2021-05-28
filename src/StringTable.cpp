#include "StringTable.hpp"

StringTable::StringTable(persist::shared_memory & mem) :
    strings(persist::fast_allocator<char>(mem)),
    ids({}, std::hash<string_type>(), std::equal_to<string_type>(), persist::fast_allocator<std::pair<const string_type, int>>(mem))
{
}


int StringTable::GetId(const string_type &s)
{
    auto i = ids.find(s);
    if(i == ids.end())
    {
        auto id = strings.size();
        strings.push_back(s);
        ids.insert(std::make_pair(s, id));
        return id;
    }
    else
        return i->second;
}

const string_type & StringTable::GetString(int id) const
{
    return strings[id];
}
