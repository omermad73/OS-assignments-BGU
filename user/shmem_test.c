#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define SHM_SIZE 4096

int main() {
    // parent creates data
    char *memory = malloc(SHM_SIZE);
    strcpy(memory, "Initial data");

    int pid = fork();

    if (pid != 0) {
        // parent process
        // parent maps its memory to child
        uint64 child_mapped_addr = map_shared_pages(pid, (uint64)memory, SHM_SIZE);
        if (child_mapped_addr == 0) {
            printf("Parent: mapping failed\n");
            exit(1);
        }
        printf("Parent: mapped memory to child at %p\n", (void*)child_mapped_addr);

        // Wait for child to finish writing
        wait(0);

        // parent reads the message child wrote
        printf("Parent reads: \"%s\"\n", memory);

        free(memory);

    } else {
        // child process
        // Get size before parent maps anything
        uint64 size_before = (uint64)sbrk(0);
        printf("Child size before mapping: %d\n", (int)size_before);

        // Wait for parent to complete mapping
        sleep(1);

        // Get size after parent mapped
        uint64 size_after = (uint64)sbrk(0);
        printf("Child size after mapping: %d\n", (int)size_after);

        // The mapped memory starts at our NEW sz minus the mapped size
        char *mapped_memory = (char*)(size_after - SHM_SIZE);

        printf("Child: trying to access mapped memory at %p\n", mapped_memory);

        // child writes the message
        strcpy(mapped_memory, "Hello daddy");
        printf("Child wrote: \"%s\"\n", mapped_memory);

        // child unmaps
         if (unmap_shared_pages(getpid(),(uint64)mapped_memory, SHM_SIZE) == 0) {
             printf("Child: unmapped successfully\n");

             uint64 size_after_unmap = (uint64)sbrk(0);
             printf("Child size after unmap: %d\n", (int)size_after_unmap);

             // Test malloc
             char *test = malloc(100);
             if (test) {
                 strcpy(test, "malloc works");
                 printf("Child: malloc test: %s\n", test);
                 free(test);
             }
        }

        exit(0);
    }
    return 0;
}
