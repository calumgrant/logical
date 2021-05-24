// Copyright (C) Calum Grant 2003
// Copying permitted under the terms of the GNU Public Licence (GPL)
//
// Defines common STL containers using the persist::alloc allocator

#ifndef _PERSIST_STL_H
#define _PERSIST_STL_H
#include "persist.h"

#include <vector>
#include <string>
#include <map>
#include <set>
#include <list>

// Ideally, hash_maps would standardly implemented
// Sadly, they are not

#ifdef _MSC_VER
// This is a microsoft compiler
#include <hash_map>
#include <hash_set>
#define microsoft_stl 1
#else
// Default to the sgi-based stl
#include <ext/hash_map>
#include <ext/hash_set>
#define sgi_stl 1
#endif


namespace persist
{
    template<class T>
    class list : public std::list<T, persist::allocator<T> >
    {
    };
    
    template<class C, class Traits = std::char_traits<C> >
    class basic_string : public std::basic_string<C, Traits, persist::allocator<C> >
    {
        void operator=(const std::basic_string<C> &s);
    public:
        basic_string() { }
        basic_string(const C*c) : std::basic_string<C, Traits, persist::allocator<C> >(c) { }

        basic_string(const std::basic_string<C> &s)
        {
            assign(s.begin(), s.end());
        }

        basic_string &operator=(const C *s)
        {
            assign(s);
            return *this;
        }
        // TODO: Other constructors
    };

    typedef basic_string<char> string;
    typedef basic_string<wchar_t> wstring;
}


namespace persist
{
    template<class T>
    class vector : public std::vector<T, persist::allocator<T> >
    {
        // TODO: constructors
    };

    template<class T, class L = std::less<T> >
    class set : public std::set<T, L, persist::allocator<T> >
    {
    };

    template<class T, class L = std::less<T> >
    class multiset : public std::multiset<T, L, persist::allocator<T> >
    {
    };

    template<class T, class V, class L = std::less<T> >
    class map : public std::map<T, V, L, persist::allocator<std::pair<T,V> > >
    {
    };

    template<class T, class V, class L = std::less<T> >
    class multimap : public std::multimap<T, V, L, persist::allocator<std::pair<T,V> > >
    {
    };

#if microsoft_stl
    template<class T, class H = std::hash_compare<T, std::less<T> > >
    class hash_set : public stdext::hash_set<T, H, persist::allocator<T> >
    {
    };

    template<class K, class V, class H = std::hash_compare<K, std::less<K> > >
    class hash_map : public stdext::hash_map<K, V, H, persist::allocator<std::pair<K, V> > >
    {
    };

    template<class T, class H = std::hash_compare<T, std::less<T> > >
    class hash_multiset : public stdext::hash_multiset<T, H, persist::allocator<T> >
    {
    };

    template<class K, class V, class H = std::hash_compare<K, std::less<K> > >
    class hash_multimap : public stdext::hash_multimap<K, V, H, persist::allocator<std::pair<K, V> > >
    {
    };
#elif sgi_stl
    template< class K, class H = __gnu_cxx::hash<K>, class E = __gnu_cxx::equal_to<K> >
    class hash_set : public __gnu_cxx::hash_set< K, H, E, persist::allocator<K> >
    {
    };

    template<class K, class V, class H = __gnu_cxx::hash<K>, class E=__gnu_cxx::equal_to<K> > 
    class hash_map : public __gnu_cxx::hash_map<K, V, H, E, persist::allocator<V> > 
    {
    };

    template< class K, class H = __gnu_cxx::hash<K>, class E = __gnu_cxx::equal_to<K> >
    class hash_multiset : public __gnu_cxx::hash_multiset< K, H, E, persist::allocator<K> >
    {
    };

    template<class K, class V, class H = __gnu_cxx::hash<K>, class E=__gnu_cxx::equal_to<K> > 
    class hash_multimap : public __gnu_cxx::hash_multimap<K, V, H, E, persist::allocator<V> > 
    {
    };
#endif

    // owner
    // An owner is a pointer to a persistent object
    // It's like an auto_ptr, but less broken, 
    // and works with persistent data and the STL
    template<class T, class A = persist::allocator<T> >
    class owner
    {
        T *ptr;

    public:
        owner() : ptr(0) { }
        owner(T *p) : ptr(p) { }
        owner(const owner<T,A>& o)
        {
            if(this != &o)  // Why copy to self?
            {
                // Wrest the pointer from o, even though it is const
                ptr = o.ptr;
                const_cast<owner<T,A>&>(o).ptr = 0;
            }
        }

        ~owner() { destroy(); }

        owner<T,A> &operator=(T *p)
        {
            destroy();
            ptr = p;
            return *this;
        }

        void create()
        {
            destroy();
            ptr = new T;
        }

        void create(const T &c)
        {
            destroy();
            ptr = new T(c);
        }

        void destroy() 
        { 
            if(ptr)
            {
                A().destroy(ptr); 
                A().deallocate(ptr, 1); 
                ptr = 0;
            }
        }

        owner<T,A> &operator=(const owner<T,A>&o)
        {
            if(this != &o)
            {
                // Wrest the pointer from o, even though it is const
                ptr = o.ptr;
                const_cast<owner<T,A>&>(o).ptr = 0;
            }
            return *this;
        }

        T *operator->() const { return ptr; }
        T &operator*() { return *ptr; }
        const T &operator*() const { return *ptr; }

        // Returns the pointer
        T *get() const { return ptr; }

        // Returns the pointer, and releases it from the owner.
        // The caller must destroy this pointer.
        T *release() const { T *p = ptr; ptr=0; return p; }
    };

    // fixed_string is a fixed-length string
    template<int N, class C=char>
    class fixed_string
    {
        unsigned char len;  // The actual length of the string
        C str[N+1];
    public:
        typedef C *iterator;
        typedef const C *const_iterator;

        fixed_string() { clear(); }
        fixed_string(const_iterator d) { assign(d); }

        template<int M>
        fixed_string(const fixed_string<M,C> &b)
        {
            assign(b.begin(), b.end());
        }

        fixed_string<N,C> &operator=(const_iterator d)
        {
            assign(d);
            return *this;
        }

        template<int M>
        fixed_string<N,C> &operator=(const fixed_string<M,C> &b)
        {
            assign(b.begin(), b.end());
            return *this;
        }

        iterator begin() { return str; }
        const_iterator begin() const { return str; }

        iterator end() { return str+len; }
        const_iterator end() const { return str+len; }

        const_iterator c_str() const { return begin(); }

        size_t size() const { return len; }

        void clear() { len=0; str[len]=0; }

        void assign(const_iterator a)
        {
            for(len=0; len<N && *a; ++a, ++len)
                str[len] = *a;

            str[len]=0;  // Zero-terminate
        }

        void assign(const_iterator a, const_iterator b)
        {
            for(len=0; len<N && a!=b; ++a, ++len)
                str[len] = *a;

            str[len]=0; // Zero-terminate
        }

        C &operator[](size_t n) { return str[n]; }
        const C&operator[](size_t n) const { return str[n]; }

        template<int M>
        bool operator==(const fixed_string<M,C> &b) const
        {
            if(size() != b.size()) return false;

            for(unsigned p=0; p<size(); ++p)
            {
                if(str[p] != b[p]) return false;
            }
            return true;
        }

        bool operator==(const_iterator b) const
        {
            for(unsigned p=0; p<=size(); ++p)
            {
                if(str[p] != b[p]) return false;
            }

            return true;
        }

        template<int M>
        bool operator<(const fixed_string<M,C> &b) const
        {
            for(const C* pa = begin(), *pb = b.begin(); *pb; ++pa, ++pb)
            {
                if(*pa < *pb) return true;
                else if(*pa > *pb) return false;
            }       

            return false;
        }
    };
 }

#endif


#if microsoft_stl
namespace std
{
    template<class T>
    class hash_compare<persist::basic_string<T>, std::less<persist::basic_string<T> > >
    {
    public:
        static const size_t bucket_size = 4;
        static const size_t min_buckets = 8;

        typedef persist::basic_string<T> key_type;

        size_t operator()(const key_type &s) const
        {
            size_t h=0;
            for(typename key_type::const_iterator x=s.begin(); x!=s.end(); ++x)
                h = h*5 + *x;
            return h;
        }

        bool operator()(const key_type &s1, const key_type &s2)
        {
            return s1<s2;
        }
    };

    template<int N, class C>
    class hash_compare<persist::fixed_string<N,C>, std::less<persist::fixed_string<N,C> > >
    {
    public:
        static const size_t bucket_size = 4;
        static const size_t min_buckets = 8;

        typedef persist::fixed_string<N,C> key_type;

        size_t operator()(const key_type &s) const
        {
            size_t h=0;
            for(typename key_type::const_iterator x=s.begin(); x!=s.end(); ++x)
                h = h*5 + *x;
            return h;
        }

        bool operator()(const key_type &s1, const key_type &s2)
        {
            return s1<s2;
        }
    };
}

#elif sgi_stl
namespace __gnu_cxx
{
    template<class T>
    class hash<persist::basic_string<T> >
    {
        typedef persist::basic_string<T> key_type;
    public:
        size_t operator()(const key_type &s) const
        {
            size_t h=0;
            for(typename key_type::const_iterator x=s.begin(); x!=s.end(); ++x)
                h = h*5 + *x;
            return h;
        }
    };
}
#endif
