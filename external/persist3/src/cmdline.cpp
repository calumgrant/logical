// cmdline.cpp
// Stores a vector of strings (the command line) and redisplays them

#include <iostream>
#include <persist_stl.h>
using namespace persist;

class History
{
public:
    vector<string> commands;   // This is persist::vector<persist::string>
};

using namespace std;


int main(int argc, char*argv[])
{
    // Declare the map: A History object is stored in the file "history.map"
    map_data<History> history("history.map");

    if(history)     // Check that the map was successfully created
    {
        try
        {
            if(argc==2 && strcmp(argv[1], "erase")==0)
            {
                // Erase the history
                history->commands.clear();
            }
            else
            {
                // Add the current command line to the history

                std::string line = argv[0];
                for(int i=1; i<argc; ++i)
                    line += " ", line += argv[i];

                history->commands.push_back(line);
            }

            // Display the history

            for(unsigned l=0; l<history->commands.size(); ++l)
                cout << history->commands[l] << endl;
        }
        catch(bad_alloc)
        {
            cout << "Memory allocation failed\n";
            return 2;
        }
    }
    else
    {
        cout << "Could not create map\n";
        return 1;
    }

    return 0;
}


// Junk below here:

#if 0
#include "persist_stl.h"
#include <iostream>

using namespace persist;

class AppData
{
public:
    struct Email
    {
        string address, public_key;
        bool encrypt;   // This person prefers encrypted email
        bool plainText; // This person prefers plaintext
    };

    struct WebPage
    {
        string url, title;
    };

    int win_x, win_y, win_height, win_width;    // Where the window was last placed

    map<string, Email> contacts;    // List of email contacts
    vector<WebPage> favourites;     // List of favourite web-pages
};


int main(int argc, char*argv[])
{    
    map_data<AppData> appdata("browser.map");

    if(appdata)     // Checks that the data was initialized properly
    {
        // Add a dummy contact

        AppData::Email mike;
        mike.address = "mike@yahoo.com";
        mike.encrypt = false;
        mike.plainText = false;

        appdata->contacts["Mike Smith"] = mike;
        std::cout << "Contact added\n";
        return 0;
    }
    else
    {
        std::cout << "Could not initialize map\n";
        return 1;
    }
}
#endif


#if 0

#include "persist.h"
#include "persist_stl.h"

#include <iostream>
#include <cassert>

using namespace persist;

class MyInfo
{
public:
    MyInfo() 
    { 
        std::cout << "Construct MyInfo\n"; 
#if 1
        for(int i=0; i<10000; ++i)
            seq.insert(std::pair<int,int>(i,i+1));
#endif
        strings.insert("Hello!");
    }
    // ~MyInfo() { std::cout << "Destroy MyInfo\n"; }
    persist::string name;
    persist::vector<persist::string> names, names2;
    persist::vector<persist::string> dummy;
    map<int, int> seq;
    
    set<int> data3;
    // TODO

    hash_set<persist::string> strings;

    // vector<owner<class MyTest> > tests;

    void test()
    {
#if 1
        for(int i=0; i<10000; ++i)
        {
            lock l;
            assert(seq[i] == i+1);
        }
#endif
    }
};


// using namespace std;

void run(int argc, char **argv)
{
    // map_file file("test.map");
    // map_root<MyInfo> root1(file); 

    map_data<MyInfo> root1("test.map");

    if(root1)
    {
        root1->test();

        if(argc==2 && strcmp(argv[1], "erase")==0)
        {
            root1->names.clear(), root1->names2.clear();
            return;
        }

        persist::string cmdLine; // = "x";  // = argv[0];
        for(int i=1; i<argc; ++i)
        {
            if(i>1) cmdLine += " ";
            cmdLine += argv[i];
        }

        root1->names.push_back(cmdLine);
        root1->names2.push_back(cmdLine);

        for(unsigned i=0; i<root1->names.size(); ++i)
            assert(root1->names[i] == root1->names2[i]);

        for(persist::vector<persist::string>::iterator i=root1->names.begin(); i!=root1->names.end(); ++i)
        {
            std::cout << i->c_str() << std::endl;
        }
    }
    else
        cout << "Failed to create map\n";
}


int main(int argc, char* argv[])
{
    try
    {
        run(argc, argv);
    }
    catch(std::bad_alloc)
    {
        cout << "Out of memory!\n";
    }
	return 0;
}
#endif


