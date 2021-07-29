#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include "Allocator.hpp"

// TODO: Move this into separate file

// A very simple string representation
// This does not manage memory or deallocate the string
// The string is null-terminated
class StringRef
{
public:
    typedef const char * iterator;
    typedef unsigned size_type;
    typedef char value_type;
    typedef const value_type * pointer_type;

    StringRef(pointer_type data, size_type len) : p(data), length(len) { }
    
    iterator begin() const { return p; }
    iterator end() const { return p+length; }
    pointer_type data() const { return p; }
    pointer_type c_str() const { return p; }
    size_type size() const { return length; }

private:
    pointer_type p;
    size_type length;
};

inline std::ostream & operator<<(std::ostream & os, const StringRef & str)
{
    for(auto c : str)
        os << c;
    return os;
}

class StringTable
{
public:
    StringTable(AllocatorData &mem);
    ~StringTable();
    
    typedef int ID;

    ID GetId(const char * value);
    ID GetId(const StringRef & len);
    const char* GetString(ID id) const;
    StringRef GetStringRef(ID id) const;
    
private:
    PERSIST_ALLOCATOR<char> allocator;
    
    std::vector<StringRef, PERSIST_ALLOCATOR<StringRef>> strings;
    
    struct Hash
    {
        inline int operator()(const StringRef & len) const;
        inline bool operator()(const StringRef & s1, const StringRef &s2) const;
    };
    std::unordered_map<StringRef, ID, Hash, Hash, PERSIST_ALLOCATOR<std::pair<const StringRef, ID>>> ids;
};
