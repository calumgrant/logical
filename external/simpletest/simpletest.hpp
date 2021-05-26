#pragma once
#include <iostream>
#include <vector>

namespace Test
{
    namespace Impl
    {
        class FixtureStatic
        {
        public:
            ~FixtureStatic()
            {
                if(failures>0)
                    std::cerr << "*** " << failures << " tests failed ***\n";
                else
                    std::cout << "All tests passed\n";
                if(failures)
                    exit(1);
            }

            int failures=0, successes=0;
        };

        template<typename T>
        class FixtureBase
        {
        protected:
            static FixtureStatic stat;
        };
    }

    class Failed : public std::exception
    {
    };

    template<typename T>
    class Fixture : private Impl::FixtureBase<bool>
    {
    public:
        const char * name;

        typedef Fixture<T> base;

        Fixture()
        {
            name = typeid(T).name();
        }

        Fixture(const char * name) : name(name) { }

        ~Fixture()
        {
            if(!running)
            {
                running = true;
                int successes=0, failures=0;
                for(auto p : tests)
                {
                    try
                    {
                        T t;
                        (t.*p)();
                        ++successes;
                    }
                    catch(const Failed &f)
                    {
                        ++failures;
                    }
                    catch(const std::exception& e)
                    {
                        ++failures;
                    }
                    catch(...)
                    {
                        ++failures;
                    }
                }

                ++(failures>0 ? stat.failures : stat.successes);

                running = false;

                std::cout << (failures>0 ? "*** Failed" : "Passed")  << ": " << name << "\n";
            }
        }

        typedef void (T::*testfn)();

        void AddTest(testfn fn)
        {
            if(!running)
                tests.push_back(fn);
        }
    protected:
        template<typename A, typename B>
        void Equals(const A &a, const B &b, const char * file, int line)
        {
            bool p = a == b;
            if(!p)
                std::cerr << name << ": Expecting a " << a << " but got " << b << std::endl;
            Check(p, file, line);
        }

        void Check(bool result, const char * file, int line)
        {
            if(!result)
            {
                std::cerr << name << ": Test failed at " << file << ":" << line << std::endl;
                throw Failed();
            }
        }
         
    private:
        static bool running;
        std::vector<testfn> tests;
    };

    template<typename T>
    bool Fixture<T>::running = false;

    template<typename T>
    Impl::FixtureStatic Impl::FixtureBase<T>::stat;
}

#define EQUALS(A,B) {this->Equals(A,B,__FILE__,__LINE__);}
#define CHECK(A) { this->Check(A,__FILE__,__LINE__); }