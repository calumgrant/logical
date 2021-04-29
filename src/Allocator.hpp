
#include <memory>

namespace Logical
{
    /*
        This is a special allocator that allocates global data that
        persists until the end of the process. This allows Logical
        to use much more memory than simply core memory, which
        is important because it is an in-memory database.

        There is no free function - data is only freed at the end of the process.

        This allocator is static and not thread-safe. Due to its simplicity
        it is extremely fast.
    */
    namespace Allocator
    {
        // Set a size limit and location of the file.
        // You must call this before all calls to malloc.
        void Initialize(std::size_t size, const char * path);

        // Allocate some memory. It's aligned to 16 bytes.
        void * malloc(std::size_t size);

        // Delete the temporary file. You must not call
        // malloc after this, until you call Initialize again.
        void Cleanup();
    }
}
