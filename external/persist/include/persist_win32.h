// Copyright (C) Calum Grant 2003
// Copying permitted under the terms of the GNU Public Licence (GPL)

// This file is only included in win32 targets

namespace persist
{
    typedef unsigned __int64 offset_t;
    typedef unsigned page_t;

    const int default_map_address = 0x40000000;

    class shared_base   // win32 version
    {
    protected:
        void *hFile;
        void *hMapFile;
        void *hUserMutex;
        void *hMemoryMutex;
        void *hEvent;

        const char *sharename;
        int mapFlags;
    };
}


