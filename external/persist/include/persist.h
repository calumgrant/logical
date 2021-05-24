// Copyright (C) Calum Grant 2003
// Copying permitted under the terms of the GNU Public Licence (GPL)

#ifndef PERSIST_H
#define PERSIST_H

#ifdef _WIN32
#include "persist_win32.h"
#else
#include "persist_unix.h"
#endif

#include <memory>

namespace persist
{
    enum { shared_heap=1, private_map=2, auto_grow=4, temp_heap=8, create_new=16, read_only=32 };

    // map_file
    // A wrapper around a block of shared memory.
    // This provides memory management functions, locking, and
    // extends the heap when necessary.
    class map_file : public shared_base
    {
        struct shared_data *map_address;
        size_t mapped_size;
        void *base_address;
        int flags;

        void extend_mapping(size_t size);
        void remap();
        void map(size_t size);
        void unmap();
        void lockMem();
        void unlockMem();

    public:

        map_file(const char *filename, 
            size_t length=16384, 
            int flags = auto_grow,
            size_t base=default_map_address);   

        ~map_file();

        void open(const char *filename, 
            size_t length=16384, 
            int flags = auto_grow,
            size_t base=default_map_address);

        void close();

        void *malloc(size_t);
        void free(void*, size_t);

        bool lock(int ms=0);    // Mutex the entire heap
        void unlock();          // Release the entire heap

        bool wait(int ms=0);    // Wait for event
        void signal();          // Signal event

        void *root() const;     // The root object
        void select(int seg);   // Makes the given segment usable

        static map_file *global;  // The global pointer

        // Returns true if the heap is empty: no objects have yet been created
        bool empty() const;

        // Returns true if the heap is valid and usable
        operator bool() const { return map_address!=0; }

    };


    // lock
    // A simple lock on the entire file
    class lock
    {
    public:
        lock(int t=0) { map_file::global->lock(t); }
        ~lock() { map_file::global->unlock(); }
    };
    

    // shared_alloc
    // An allocator compatible with the STL
    // This allocator uses the default map_file.  
    // We don't actually include a reference to the shared file here, since that 
    // would get stored in persistent memory.
    template<class T>
    class allocator : public std::allocator<T>
    {
    public:
        // Construct from another allocator
        template<class O>
        allocator(const allocator<O>&) { }

        allocator() { }

        typedef T value_type;
        typedef const T *const_pointer;
        typedef T *pointer;
        typedef const T &const_reference;
        typedef T &reference;
        typedef typename std::allocator<T>::difference_type difference_type;
        typedef typename std::allocator<T>::size_type size_type;

        pointer allocate(size_type n)
        {
            if(!map_file::global) throw std::bad_alloc();
            pointer p = static_cast<pointer>(map_file::global->malloc(n * sizeof(T)));
            if(!p) throw std::bad_alloc();

            return p;
        }

        void deallocate(pointer p, size_type count)
        {
            if(map_file::global)
                map_file::global->free(p, count * sizeof(T));
        }

        size_type max_size() const
        {
            if(!map_file::global) return 0;

            return -1;
        }

        void construct(pointer p, const_reference v)
        {
            std::allocator<T>::construct(p,v);
        }

        void destroy(pointer p)
        {
            std::allocator<T>::destroy(p);
        }

	    template<class Other>
		struct rebind
		{
            typedef persist::allocator<Other> other;
		};
    };


    // map_data
    // A type-safe wrapper around the root object of a map_file
    // It also constructs a new object when the file is empty
    template<class T>
    class map_data : public map_file
    {
    public:
        map_data(const char *filename, 
            size_t length=16384, 
            int flags = auto_grow,
            size_t base=default_map_address) :
            map_file(filename, length, flags, base)
        {
            if(empty())
            {                
                new(*this) T();
            }
        }

        T &operator*()
        {
            return *static_cast<T*>(root());
        }

        T *operator->()
        {
            return static_cast<T*>(root());
        }
    };
}


void *operator new(size_t size, persist::map_file&share);
void operator delete(void *p, persist::map_file&share);

#endif
