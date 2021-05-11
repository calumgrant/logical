
#include <vector>
#include <unordered_set>

template<typename T, typename Hash = std::hash<T>, typename Alloc = std::allocator<T> >
class TableXX
{
public:

    typedef Alloc allocator_type;
    typedef Hash hash_type;
    typedef T column_type;
    typedef const column_type* row_type;
    typedef std::size_t size_type;

    TableXX(size_type columns);

    void add(row_type d)
    {
        data.insert(data.back(), d, d+m_arity);
    }

    bool contains(row_type) const;

    struct Hasher
    {
        
        int operator()(size_type row) const
        {

        }

        bool operator()(size_type row1, size_type row2)
        {
            return row1 < row2;
        }
    };

    std::unordered_multiset<size_type> index;

    class Index
    {
    public:
        Index(size_type columns);
    };

private:
    size_type m_arity;
    hash_type hash;
    std::vector<column_type, Alloc> data;
};
