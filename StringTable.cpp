#include "StringTable.hpp"

int StringTable::GetId(const std::string &s)
{
    auto x = ids.find(s);
    if(x == ids.end())
    {
        auto id = strings.size();
        strings.push_back(s);
        ids.insert(std::make_pair(s, id));
        return id;
    }
    else
    return x->second;
}

const std::string & StringTable::GetString(int id)
{
    return strings[id];
}
