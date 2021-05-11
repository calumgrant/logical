#include "CompoundName.hpp"

CompoundName::CompoundName(const std::vector<int> & list) : parts(list)
{
    std::sort(parts.begin(), parts.end());
    auto end = std::unique(parts.begin(), parts.end());
    parts.erase(end, parts.end());
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
