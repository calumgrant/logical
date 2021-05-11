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
private:
    std::vector<int> parts;
};
