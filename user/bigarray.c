#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define SIZE (1 << 16)

int
main(int argc, char *argv[])
{
  static int arr[SIZE];
  int i;
  for(i = 0; i < SIZE; i++){
    arr[i] = i;
  }

  // creating n child processes
  int n = 4;
  int pids[n];
  int ret = forkn(n, pids);
  if(ret < 0){
    printf("forkn(%d) failed\n", n);
    exit(-1, "forkn error");
  }

  // If we are a child
  if(ret > 0 && ret <= n){
    int child_index = ret - 1;
    // Each child handles a quarter of the array
    int start = (SIZE / n) * child_index;
    int end   = start + (SIZE / n);

    int partial_sum = 0;
    for(i = start; i < end; i++){
      partial_sum += arr[i];
    }

    printf("Child %d (PID %d): partial sum = %d\n", ret, getpid(), partial_sum);
    exit(partial_sum, "");
  }

  // If we reeach here we are the parent
  // Print the PIDs of the children
  printf("Parent process: created children with PIDs = ");
  for(i = 0; i < n; i++){
    printf("%d ", pids[i]);
  }
  printf("\n");

  // Wait for all child processes
  int child_count;
  int statuses[n];
  if(waitall(&child_count, statuses) < 0){
    printf("waitall error\n");
    exit(-1, "waitall error");
  }
  if(child_count != n){
    printf("Expected %d children, got %d\n", n, child_count);
    exit(-1, "");
  }

  // Sum all the partial sums
  int total_sum = 0;
  for(i = 0; i < child_count; i++){
    total_sum += statuses[i];
  }

  printf("All children finished. Combined sum = %d\n", total_sum);
  exit(0, "sum done");
}
