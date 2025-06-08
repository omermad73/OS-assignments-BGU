#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define BUFFER_SIZE 4096
#define NUM_CHILDREN 4
#define MESSAGES_PER_CHILD 100

// Convert decimal to string, returns length written
static int itoadec(int x, char *s) {
  int n = x, len = 0;
  do { len++; n /= 10; } while(n);
  s += len;

  do {
    *--s = '0' + (x % 10);
    x /= 10;
  } while(x);
  return len;
}

// Message header: 16 bits for child index + 16 bits for message length
typedef struct HEADER {
  uint16 child_idx;
  uint16 msg_len;
} msg_header_t;

// Find next 4-byte aligned address
uint64 align4(uint64 addr) {
  return (addr + 3) & ~3;
}

// Write a message to the buffer
int write_message(char *buffer, uint16 child_idx, char *msg, int msg_len) {
  uint64 offset = 0;
  uint32 header = (child_idx << 16) | (msg_len & 0xFFFF);

  while (offset + 4 + msg_len <= BUFFER_SIZE) {
    uint32 *hp = (uint32*)(buffer + offset);
    uint32 old = __sync_val_compare_and_swap(hp, 0, header);

    if (old == 0) {
      // claimed the slot - now write the message
      char *dest = buffer + offset + 4;
      for (int i = 0; i < msg_len; i++) {
        dest[i] = msg[i];
      }
      return 1;
    }

    // Skip this message
    uint16 used_len = old & 0xFFFF;
    offset += 4 + used_len;
    offset = align4(offset); // Align to next 4-byte boundary
  }

  return 0; // Buffer full
}

int main() {
  // Allocate shared buffer using malloc
  char *buffer = malloc(BUFFER_SIZE);
  if (!buffer) {
    printf("Failed to allocate buffer\n");
    exit(1);
  }

  // Initialize buffer to zeros
  for (int i = 0; i < BUFFER_SIZE; i++) {
    buffer[i] = 0;
  }

  printf("Parent: created buffer at %p\n", buffer);
  int pids[NUM_CHILDREN];

  // Fork children
  for (int i = 0; i < NUM_CHILDREN; i++) {
    pids[i] = fork();

    if (pids[i] < 0) {
      printf("Fork failed\n");
      exit(1);
    }

    if (pids[i] == 0) {
      // Child process
      int child_idx = i + 1;
      printf("Child %d: started (PID %d)\n", child_idx, getpid());

      // Wait for parent to map memory
      sleep(2);

      // Get buffer location from where parent mapped it
      uint64 size_after = (uint64)sbrk(0);
      char *shared_buffer = (char*)(size_after - BUFFER_SIZE);
      printf("Child %d: buffer mapped at %p\n", child_idx, shared_buffer);

      // Try writing messages
      for (int j = 0; j < MESSAGES_PER_CHILD; j++) {
        char msg[32];
        strcpy(msg, "Message ");

        char *p = msg + 8; // After "Message "
        p += itoadec(child_idx, p);
        *p++ = '-';
        p += itoadec(j, p);
        *p = '\0';

        int msg_len = p - msg;

        if (write_message(shared_buffer, child_idx, msg, msg_len)) {
        } else {
          printf("Child %d: buffer full\n", child_idx);
          break;
        }
      }

      printf("Child %d: done\n", child_idx);
      unmap_shared_pages(getpid(), (uint64)shared_buffer, BUFFER_SIZE);
      exit(0);
    }
  }

  // Parent code
  printf("Parent: mapping buffer to children\n");

  // Map buffer to each child
  for (int i = 0; i < NUM_CHILDREN; i++) {
    uint64 child_addr = map_shared_pages(pids[i], (uint64)buffer, BUFFER_SIZE);
    if (child_addr == 0) {
      printf("Parent: failed to map to child %d\n", i + 1);
    } else {
      printf("Parent: mapped buffer to child %d at %p\n", i + 1, (void*)child_addr);
    }
  }

  // sleep to allow children to write some messages
  sleep(3);

  // Scan and print messages
  printf("=== Reading log messages ===\n");
  uint64 offset = 0;

  while (offset + 4 <= BUFFER_SIZE) {
    uint32 hdr = *(uint32*)(buffer + offset);

    if (hdr != 0) {
      uint16 child_idx = hdr >> 16;
      uint16 msg_len = hdr & 0xFFFF;

      char message[100]; // Temp buffer for message copy
      for (int i = 0; i < msg_len && i < 99; i++) {
        message[i] = buffer[offset + 4 + i];
      }
      message[msg_len < 99 ? msg_len : 99] = '\0'; // Ensure null termination

      printf("Child %d: %s\n", child_idx, message);
      offset += 4 + msg_len;
    } else {
      offset += 4;
    }

    offset = align4(offset);
  }
  printf("=== End of log messages ===\n");

  return 0;
}
