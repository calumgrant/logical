#pragma once
#include <vector>

class CompoundName
{
public:
    CompoundName(std::vector<int> && parts);
    CompoundName(const std::vector<int> & parts);
    
    bool operator==(const CompoundName & other) const;
    
    // Whether we are contained completely in another name
    bool operator<=(const CompoundName & other) const;
    
    struct Hash
    {
        int operator()(const CompoundName &n) const
        {
            int h = 0;
            for(auto i : n.parts)
                h = h*13 + i;
            return h;
        }
    };

    std::vector<int> parts;
};
