
#include "persist.h"
#include "shared_data.h"

#include <iostream>  // tmp
using namespace std; // tmp

#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

using namespace persist;


// constructor::map_file
//
// Opens the file to get the file descriptor (fd).  Then calls mmap to map the file to memory.
// It first read the first part of the file, and the might need to remap the file to the 
// location specified by the first invocation of the map.

map_file::map_file(const char *filename, size_t length, int f, size_t base)
{
    map_address = 0;
    base_address = (void*)base;
    flags = f;
    fd = -1;

    open(filename, length, f, base);
}

void map_file::open(const char *filename, size_t length, int f, size_t base)
{
    close();
    // mem_mutex = PTHREAD_MUTEX_INITIALIZER;
    // user_mutex = PTHREAD_MUTEX_INITIALIZER;

    pthread_mutex_init(&mem_mutex, 0);
    pthread_mutex_init(&user_mutex, 0);

    if(flags & private_map) mapFlags = MAP_PRIVATE|MAP_FIXED;
    else mapFlags =  MAP_SHARED|MAP_FIXED;

    fd = ::open(filename, O_RDWR, S_IRWXU|S_IRGRP|S_IROTH);

    if(fd == -1)
    {
        // File did not exist, so we create it instead

        fd = ::open(filename, O_CREAT|O_RDWR, S_IRWXU|S_IRGRP|S_IROTH);

        if(fd == -1) return;  // Failed to create file

        // Fill up the file with zeros

        char c=0;
        lseek(fd, length-1, SEEK_SET);
        write(fd, &c, 1);
    }

    // Seek to the end of the file: we need to ensure enough of the file is allocated
    if(base_address == 0) mapFlags -= MAP_FIXED;

    char *addr = (char*)mmap((char*)base_address, length, PROT_WRITE|PROT_READ, mapFlags, fd, 0);

    if(addr == MAP_FAILED)
        map_address = 0;
    else
        map_address = (shared_data*)addr;

    if(base_address == 0)
    {
        mapFlags += MAP_FIXED;
        base_address = map_address;
    }

    if(map_address)
    {
        // Remap the file according to the specifications in the header file

        void *previous_address = map_address->address;
        size_t previous_length = map_address->length;

        if(previous_address && (previous_length != length || previous_address != map_address))
        {
             munmap((char*)map_address, length);
             length = previous_length;
             base_address = previous_address;

             addr = (char*)mmap((char*)base_address, length, PROT_WRITE|PROT_READ, mapFlags, fd, 0);

             if(addr == MAP_FAILED)
                map_address = 0;
             else
             {
                map_address = (shared_data*)addr;        
             }

        }
        else
            base_address = map_address;
    }

    if(map_address)
    {
        mapped_size = length;

        if(map_address->address)
        {
            if(map_address->address != map_address)
            {
                // This is a failure!

                munmap((char*)map_address, length);
                map_address = 0;
            }
        }
        else
        {
            // This is a new address
            map_address->address = map_address;
            map_address->length = length;
            map_address->root = map_address+1;
            map_address->end = (char*)map_address + length;
            map_address->top = (char*)map_address->root;
            map_address->auto_grow = flags&auto_grow;

            // This is not needed
            for(int i=0; i<64; ++i) map_address->free_space[i] = 0;
        }
    }

    if(base_address == 0) base_address = map_address;
    if(map_address) assert(map_address == base_address);

    // Report on where it ended up
    // std::cout << "Mapped to " << map_address << std::endl;

    global = this;
}



map_file::~map_file()
{
    close();
}


void map_file::close()
{
    unmap();
    ::close(fd);
    fd = -1;
}


void map_file::unmap()
{
    if(map_address)
    {
        munmap((char*)map_address, mapped_size);
        map_address = 0;
    }
}

void map_file::map(size_t new_size)
{
    if(!map_address)
    {
        char *m = (char*)mmap((char*)base_address, new_size, PROT_WRITE|PROT_READ, mapFlags, fd, 0);

        if(m != MAP_FAILED)
        {
            map_address = (shared_data*)base_address;
            mapped_size = new_size;
        }
    }
}


void map_file::remap()
{
    if(mapped_size < map_address->length)
    {
        // Another process has increased the shared memory
        // we must therefore increase our own usage

        // TODO: remap

        size_t new_length = map_address->length;
        unmap();
        map(new_length);
    }
}


#define MREMAP 0    // 1 on Linux

void map_file::extend_mapping(size_t extra)
{
    // assert(map_address == base_address);
    size_t old_length = map_address->length;

    if(extra<16384) extra = 16384;
    if(extra < old_length/2) extra = old_length/2;

    size_t new_length = old_length + extra;

    // extend the file a bit
    char c=0;
    lseek(fd, new_length-1, SEEK_SET);
    write(fd, &c, 1);

#if MREMAP
    void *new_address = mremap((char*)map_address, old_length, new_length, 0);  // Do NOT relocate it

    if(new_address)
    {
        // remap successful
        map_address->length = new_length;
    }
#else

    munmap((char*)map_address, old_length);

    char *m = (char*)mmap((char*)map_address, new_length, PROT_WRITE|PROT_READ, mapFlags, fd, 0);

    if(m == MAP_FAILED)
    {
        // Go back to the original map then
        char *m = (char*)mmap((char*)map_address, old_length, PROT_WRITE|PROT_READ, mapFlags, fd, 0);
        assert(m != MAP_FAILED);
        assert(m == (char*)map_address);
    }
    else
    {
        assert(m == (char*)map_address);
        mapped_size = new_length;
        map_address->length = new_length;
        map_address->end = (char*)map_address + new_length;
    }

#endif
    assert(map_address == base_address);
}


bool map_file::lock(int ms)
{
    remap();

    pthread_mutex_lock(&user_mutex);
    return true;
}


void map_file::unlock()
{
    pthread_mutex_unlock(&user_mutex);
}

void map_file::lockMem()
{
    pthread_mutex_lock(&mem_mutex);    
}


void map_file::unlockMem()
{
    pthread_mutex_unlock(&mem_mutex);
}
