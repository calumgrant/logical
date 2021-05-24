// Copyright (C) Calum Grant 2003
// Copying permitted under the terms of the GNU Public Licence (GPL)

namespace persist
{
    // This data structure is at the very start of the mapped file/memory
    // It contains the parameters of the mapping, and the list of free memory
    struct shared_data
    {
        shared_data *address;   // The address we expect to be at - we need to reopen if this fails

        size_t length;          // The size of the allocation
        bool auto_grow;         // Whether this length should be increased on demand
        size_t segment_size;

        // pthread_mutex_t mutex;   // For shared data

        void *condition;

        void *root;
        char *top;
        void *end;

        void *free_space[64];   // An embarrassingly simple memory manager
    };
}
