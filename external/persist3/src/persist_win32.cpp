// Copyright (C) Calum Grant 2003
// Copying permitted under the terms of the GNU Public Licence (GPL)
//
// File specifically for windows

#include <windows.h>

#include "persist.h"
#include "shared_data.h"

#include <cassert>
#include <iostream>

using namespace std;
using namespace persist;

unsigned maxMapSizeHi = 0;
unsigned maxMapSizeLo = 0x40000000;

// constructor: map_file
//
// Initialises the file map.  When initialization fails, map_address is 0.
// Otherwise, it points to the mapped memory.
// The various hocus-pocus required to get Windows to map some memory.
// We first map the start of the file, and read the previous map parameters
// if they differ, we remap the memory to the previous location and size.

map_file::map_file(const char *filename, size_t length, int f, size_t base)
{
    map_address = 0;
    hFile = INVALID_HANDLE_VALUE;

    open(filename, length, f, base);

}


// destructor: map_file
//
// Unmaps the memory and closes all the handles we have previously created.

map_file::~map_file()
{
    close();
    global = 0;
}


void map_file::open(const char *filename, size_t length, int f, size_t base)
{
    close();

    base_address = (void*)base;
    sharename = 0;  // Unused currently - todo get rid of this
    flags = f;

    if(flags & read_only) mapFlags = PAGE_READONLY;
    else if(flags & private_map) mapFlags = PAGE_WRITECOPY;
    else mapFlags = PAGE_READWRITE;

    if(!sharename) sharename = filename;

    // VirtualAlloc(base_address, 0x80000000, MEM_RESERVE, 0);
    // Make sure nothing else uses this memory, so we can extend it at leisure

    hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, sharename);

    // hMapFile = 0;

    if(!hMapFile)
    {
        hFile = CreateFile(filename, 
            FILE_ALL_ACCESS, FILE_SHARE_WRITE|FILE_SHARE_READ, 0, 
            OPEN_ALWAYS, 
            FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,
            0);

        if(hFile == INVALID_HANDLE_VALUE)
        {
            // Can't proceed
            return; 
        }

        // The file does not exist
        hMapFile = CreateFileMapping(hFile, 0, mapFlags, 0, sizeof(shared_data), sharename);
    }

    if(hMapFile)
    {
        map_address = (shared_data*)MapViewOfFileEx(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(shared_data), base_address);

        if(map_address)
        {
            shared_data *previous_address = map_address->address;
            size_t previous_length = map_address->length;
    
            unmap();

            // Remap the entire file

            if(previous_address)
            {
                length = previous_length;
                base_address = previous_address;
            }

            map(length);
        }
    }
    else 
    {
        perror("Could not create file map\n");
        return;  // Failed
    }

    if(map_address)
    {
        // Everything well
        if(map_address->address)  // Created before
        {
            if(map_address->address != map_address)
            {
                // Failed to allocate at the correct location
                UnmapViewOfFile(map_address);
                map_address = 0;
            }
        }
        else
        {
            map_address->address = map_address;
            map_address->length = length;
            map_address->end = (char*)map_address + length;
            map_address->top = (char*)(map_address+1);  // Parentheses important
            map_address->root = map_address->top;
            map_address->auto_grow = flags&auto_grow;
        }
    }
    else
        return;

    if(map_address)
    {
        // Initialize the shared mutex
        hUserMutex = CreateMutex(0, 0, "My shared mutex");      // TODO - a sensible name
        hMemoryMutex = CreateMutex(0, 0, "My memory mutex");    // TODO - a sensible name
        hEvent = CreateEvent(0, 1, 0, "My shared event");
    }

    global = this;

}


void map_file::close()
{
    unmap();

    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;

}


// map_file::unmap
//
// This removes all evidence of the map from the operating system

void map_file::unmap()
{
    if(map_address)
    {
        UnmapViewOfFile(map_address);
        CloseHandle(hMapFile);
        map_address = 0;
    }
}


// map_file::map
//
// Creates a map using the size @new_size.  The map must be created at base_address.

void map_file::map(size_t new_size)
{
    assert(!map_address);

    if(hFile == INVALID_HANDLE_VALUE)
    {
        hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, sharename);
    }
    else
    {
        hMapFile = CreateFileMapping(hFile, 0, mapFlags, 0, new_size, sharename);
    }

    if(hMapFile)
    {
        mapped_size = new_size;
        map_address = (shared_data*)MapViewOfFileEx(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, mapped_size, base_address);

        // CloseHandle(hMapFile);
        // Mapping successful if map_address != 0
    }
}


// map_file::extend_mapping
//
// Extends the mapping by @extra bytes
// This involves closing the map, then recreating it with the new size.

void map_file::extend_mapping(size_t extra)
{
    void *old_address = map_address;
    size_t old_length = map_address->length;

    if(extra<16384) extra = 16384;
    if(extra < old_length/2) extra = old_length/2;

    size_t new_length = old_length + extra;

    UnmapViewOfFile(map_address);

    CloseHandle(hMapFile);
    hMapFile = CreateFileMapping(hFile, 0, mapFlags, 0, new_length, sharename);

    map_address = (shared_data*)MapViewOfFileEx(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, new_length, old_address);            

    if(map_address)
    {
        // Successfully extended
        map_address->length = new_length;
        mapped_size = new_length;
        map_address->end = (char*)map_address + new_length;
    }
    else
    {
        // Failed to extend
        map_address = (shared_data*)MapViewOfFileEx(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, old_length, old_address);            
    }    
}


// map_file::remap
//
// This checks the mapping to see if another process has changed its size.
// If it has, we must increase our size before doing anything else


void map_file::remap()
{
    if(mapped_size < map_address->length)
    {
        // Another process has increased the shared memory
        // we must therefore increase our own usage
        assert(0);  // not implemented yet!
    }
}


// map_file::lock
//
// Locks the user mutex.  Returns true if mutex obtained

bool map_file::lock(int ms)
{
   remap();  // Make sure we haven't grown

   if(ms == 0) ms=INFINITE;

   DWORD waitresult = WaitForSingleObject(hUserMutex, ms);

   return waitresult == WAIT_OBJECT_0;  // Got ownership
}


// map_file::unlock
//
// Unlocks the user mutex.

void map_file::unlock()
{
    ReleaseMutex(hUserMutex);
}


// map_file::lockMem
//
// Locks the internal mutex used by the memory allocator

void map_file::lockMem()
{
   remap();  // Make sure we haven't grown

   WaitForSingleObject(hMemoryMutex, INFINITE);
}


// map_file::unlockMem
//
// Unlocks the internal mutex used by the memory allocator

void map_file::unlockMem()
{
    ReleaseMutex(hMemoryMutex);
}


// map_file::signal
//
// Experimental: communicate between two processes

void map_file::signal()
{
    SetEvent(hEvent);
}


// map_file::wait
//
// Wait for the event.  Experimental, unfinished, don't use.

bool map_file::wait(int ms)
{
    if(ms == 0) ms=INFINITE;

    DWORD waitresult = WaitForSingleObject(hEvent, ms);

    return waitresult == WAIT_OBJECT_0;
}
