#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define SIZE (1 << 16) // 65536

int
main(int argc, char *argv[])
{
  static int arr[SIZE];
  int i;
  for(i = 0; i < SIZE; i++){
    arr[i] = i;
  }

  // number of processes to create
  int n = 4;
  int pids[4];
  int ret = forkn(n, pids);
  if(ret < 0){
    printf("forkn(%d) failed!\n", n);
    exit(-1, "forkn error");
  }

  if(ret > 0 && ret <= n){
    // We are a child. ret is 1..4
    int child_index = ret - 1; // 0..3
    // Each child handles a quarter of the array
    int start = (SIZE / n) * child_index;
    int end   = start + (SIZE / n);

    long long partial_sum = 0;
    for(i = start; i < end; i++){
      partial_sum += arr[i];
    }

    // Print partial sum and exit with it
    // Because exit status is only 32-bit in xv6, be sure partial_sum fits
    // if partial_sum > 2^31, you'd do a trick or see assignment instructions
    printf("Child %d (PID %d): partial sum = %d\n", ret, getpid(), partial_sum);
    exit((int)partial_sum, "");
  }

  // Else ret == 0 => we are the parent
  // Print child PIDs
  printf("Parent process: created children with PIDs = ");
  for(i = 0; i < n; i++){
    printf("%d ", pids[i]);
  }
  printf("\n");

  // Wait for all child processes
  int child_count;
  // We pass in a big statuses array of size NPROC if the assignment says so
  int statuses[64];
  if(waitall(&child_count, statuses) < 0){
    printf("waitall error!\n");
    exit(-1, "waitall error");
  }
  if(child_count != n){
    printf("Expected %d children, got %d\n", n, child_count);
    exit(-1, "");
  }

  long long total_sum = 0;
  for(i = 0; i < child_count; i++){
    // Each status is the partial sum from a child
    // Because we used exit(partial_sum, ""), we should get it here
    total_sum += (long long)statuses[i];
  }

  printf("All children finished. Combined sum = %d\n", total_sum);
  exit(0, "sum done");
}
