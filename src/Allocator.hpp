#pragma once
#include <memory>

#define MMAP_ALLOCATOR 0

class MemoryCounter
{
public:
    typedef std::size_t size_type;
    
    MemoryCounter(size_type limit) : bytes_allocated(0), limit_(limit) {}
    
    void allocate(size_type delta)
    {
        auto result = (bytes_allocated += delta);
        
        if(result > limit_)
        {
            deallocate(delta);
            throw std::bad_alloc();
        }
    }
    
    void deallocate(size_type delta)
    {
        bytes_allocated -= delta;
    }
    
    size_type capacity() const { return limit_ - bytes_allocated; }
    
    void limit(size_type l)
    {
        // Must not be called in a multithreaded context.
        if(l<bytes_allocated)
            throw std::bad_alloc();
        
        limit_ = l;
    }
    
    size_type limit() const { return limit_; }
    size_type size() const { return bytes_allocated; }

private:
    // Invariant: 0<=bytes_allocated<=limit
    
    std::atomic<std::size_t> bytes_allocated;
    size_type limit_;
};

template<typename T>
class CountedAllocator
{
public:
    CountedAllocator(MemoryCounter &mc) : counter(mc) {}
    
    // Construct from another allocator
    template<class O>
    CountedAllocator(const CountedAllocator<O>&o) : counter(o.counter), alloc(o.alloc) { }

    typedef T value_type;
    typedef const T *const_pointer;
    typedef T *pointer;
    typedef const T &const_reference;
    typedef T &reference;
    typedef typename std::allocator<T>::difference_type difference_type;
    typedef typename std::allocator<T>::size_type size_type;

    pointer allocate(size_type n)
    {
        counter.allocate(n * sizeof(T));
        try
        {
            return alloc.allocate(n);
        }
        catch(...)
        {
            counter.deallocate(n * sizeof(T));
            throw;
        }
    }

    void deallocate(pointer p, size_type count)
    {
        alloc.deallocate(p, count);
        counter.deallocate(count * sizeof(T));
    }

    size_type max_size() const
    {
        return counter.capacity()/sizeof(T);
    }

    template<class Other>
    struct rebind
    {
        typedef CountedAllocator<Other> other;
    };

    MemoryCounter & counter;
    std::allocator<T> alloc;
    
    bool operator!=(const CountedAllocator & other) const
    {
        return &counter != &other.counter;
    }
};


#if MMAP_ALLOCATOR
#include "persist.h"

#define PERSIST_ALLOCATOR persist::fast_allocator

using AllocatorData = persist::shared_memory;

#else

using AllocatorData = MemoryCounter;

#define PERSIST_ALLOCATOR CountedAllocator
#endif
