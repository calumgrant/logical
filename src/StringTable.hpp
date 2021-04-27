#include <string>
#include <unordered_map>
#include <vector>

class StringTable
{
public:
    int GetId(const std::string & value);
    const std::string &GetString(int id);
private:
    std::vector<std::string> strings;
    std::unordered_map<std::string, int> ids;
};
