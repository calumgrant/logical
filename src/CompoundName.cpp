#include "CompoundName.hpp"
#include <algorithm>
#include <cassert>

CompoundName::CompoundName(const std::vector<int> & list) : parts(list)
{
    mapFromInputToOutput.reserve(parts.size());
    
    std::sort(parts.begin(), parts.end());
    auto end = std::unique(parts.begin(), parts.end());
    parts.erase(end, parts.end());
    
    for(auto i : list)
    {
        // Search for i.
        // TODO: Use a binary search to avoid quadratic algorithm
        for(int j=0; j<parts.size(); ++j)
        {
            if(i == parts[j])
            {
                mapFromInputToOutput.push_back(j);
                break;
            }
        }
    }
    
    assert(mapFromInputToOutput.size() == list.size());
}

CompoundName::CompoundName()
{
}

bool CompoundName::operator==(const CompoundName & other) const
{
    return parts == other.parts;
}

bool CompoundName::operator<=(const CompoundName & other) const
{
    // Check that all items in ourselves are also in other.
    for(auto i=parts.begin(), j=other.parts.begin(); i!=parts.end(); ++i)
    {
        while(j != other.parts.end() && *j < *i)
            ++j;
        if(j == other.parts.end() || *j>*i) return false;
    }
    return true;
}
