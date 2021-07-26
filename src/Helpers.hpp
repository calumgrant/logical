#pragma once

template<typename Key, typename Value, typename Hash = std::hash<Key>, typename Eq = std::equal_to<Key>>
struct unordered_map_helper
{
    typedef std::pair<const Key, Value> value_type;
    typedef PERSIST_ALLOCATOR<value_type> allocator_type;
    
    typedef std::unordered_map< Key, Value, Hash, Eq, allocator_type> map_type;
    typedef std::unordered_multimap< Key, Value, Hash, Eq, allocator_type> multimap_type;
};

template<typename T, typename...Args>
std::shared_ptr<T> allocate_shared(AllocatorData & map, Args&& ...args)
{
    return std::allocate_shared<T, PERSIST_ALLOCATOR<T>>(map, std::forward<Args>(args)...);
}
