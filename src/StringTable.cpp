#include "StringTable.hpp"

StringTable::StringTable(AllocatorData & mem) :
    allocator(mem),
    strings(PERSIST_ALLOCATOR<StringRef>(mem)),
    ids({}, Hash(), Hash(), PERSIST_ALLOCATOR<std::pair<const StringRef, ID>>(mem))
{
}

StringTable::~StringTable()
{
    for(auto & r : strings)
    {
        allocator.deallocate((char*)r.data(), r.size()+1);
    }
}

int StringTable::GetId(const char * str)
{
    return GetId(StringRef(str, std::strlen(str)));
}

int StringTable::GetId(const StringRef &s)
{
    auto i = ids.find(s);
    if(i == ids.end())
    {
        auto id = strings.size();
        
        auto p = allocator.allocate(s.size()+1);
        std::copy(s.begin(), s.end(), p);
        p[s.size()] = 0;
        StringRef result{p, s.size()};
        
        strings.push_back(result);
        ids.insert(std::make_pair(result, id));
        return id;
    }
    else
        return i->second;
}

const char * StringTable::GetString(int id) const
{
    return strings[id].c_str();
}

bool StringTable::Hash::operator()(const StringRef & r1, const StringRef & r2) const
{
    return std::equal(r1.begin(), r1.end(), r2.begin(), r2.end());
}

int StringTable::Hash::operator()(const StringRef & r) const
{
    int h = 0;
    for(auto c : r)
        h = h*17 + c;
    return h;
}
