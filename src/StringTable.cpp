#include "StringTable.hpp"

int StringTable::GetId(const std::string &s)
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

const std::string & StringTable::GetString(int id)
{
    return strings[id];
}
