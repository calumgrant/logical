// shared_list.cpp : Defines the entry point for the console application.
//

// #include "stdafx.h"

// #include "persist.h"
#include <iostream>
#include <ctime>
#include "persist_stl.h"

using namespace persist;

class Root
{
public:
    int number;

    Root() : number(0) { }

    int get_number()
    {
        lock l;
        return number++;
    }

    list<int> numbers;

    void write_list()
    {
        lock l;
        int n = get_number();
        numbers.push_back(n);
    }

    int read_list()
    {
        lock l;
        if(numbers.size()>0)
        {
            int n = numbers.front();
            numbers.pop_front();
            return n;
        }
        return -1;
    }

    int peek_list()
    {
        lock l;
        if(numbers.size()>0)
        {
            return numbers.front();
        }
        return -1;
    }
};

using namespace std;

int main(int argc, char* argv[])
{
    map_data<Root> root("list.map");

    if(!root)
    {
        cout << "Could not open root file\n";
        return 2;
    }

    if(argc !=2)
    {
        cout << "Usage: reader|writer|observer\n";
        return 1;
    }

    if(strcmp(argv[1], "reader")==0)
    {
        while(true)
        {
            int n = root->read_list();

            if(n%1000==0) 
                cout << n << endl;
        }
    }
    else if(strcmp(argv[1], "writer")==0)
    {
        for(int i=0; i<100000; ++i)
        {
            root->write_list();
            // cout << "Front=" << root->numbers.front() << ", back=" << root->numbers.back() << endl;
        }

        // Do the writer function
    }
    else if(strcmp(argv[1], "observer")==0)
    {
        while(true)
        // while(clock()<10000)
            cout << root->peek_list() << endl;
        // Do the observer function
    }
    else
        cout << "unknown command";

	return 0;
}

