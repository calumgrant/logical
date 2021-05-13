#pragma once
#include <vector>

class CompoundName
{
public:
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
    
    // Map from the input position to the output position.
    std::vector<int> mapFromInputToOutput;

    std::vector<int> parts;
};
