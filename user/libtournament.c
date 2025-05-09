#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/proc.h"
#include "user/user.h"

static int N;               // number of processes
static int L;               // levels = log2(N)
static int *all_locks;      // global locks array of size N-1 mapping nodes to lock IDs
static int *my_locks;       // per-process lock IDs array of size L
static int *my_roles;       // per-process roles array of size L
static int my_index;        // this process's index in [0,N)

int
node_index(int level, int idx)
{
  // BFS order: root at 0, level l nodes start at (1<<l)-1
  return ((1 << level) - 1) + (idx >> (L - level));
}

int
compute_role(int level, int idx)
{
  //return (idx >> (L - level - 1)) & 1;
  return (idx & (1 << (L - level - 1))) >> (L - level - 1);
}

int
tournament_create(int processes)
{
  if(processes <= 0 || processes > 16)
      return -1;

  int p = processes;
  if (p & (p - 1))
      return -1;

  N = processes;
  L = 0;
  int temp = processes;
  while(temp > 1) {
    temp >>= 1;
    L++;
  }

  // allocate global locks array
  all_locks = malloc((N - 1) * sizeof(int));
  // allocate per-process arrays
  my_locks = malloc(L * sizeof(int));
  my_roles = malloc(L * sizeof(int));
  if (!all_locks || !my_locks || !my_roles)
    return -1;

  for (int i = 0; i < N - 1; i++) {
      all_locks[i] = peterson_create();
      if (all_locks[i] < 0)
          return -1;
  }

  int pid;
  for (int i = 1; i < N; i++) {
    pid = fork();
    if (pid < 0)
      return -1;
    if (pid == 0) {
      my_index = i;
      break;
    }
    // parent continues looping
    if (i == N - 1)
      my_index = 0; // last parent is index 0
  }
  for (int lvl = 0; lvl < L; lvl++) {
      int idx = node_index(lvl, my_index);
      my_locks[lvl] = all_locks[idx];
      my_roles[lvl] = compute_role(lvl, my_index);
    }

  return my_index;

  // for(int i = 0; i < level; i++) {
    
  //     int fork_ret = fork();
  //     if(fork_ret == -1){
  //         return -1;
  //     }
  // }
    
}

int
tournament_acquire(void)
{
  for(int i = L - 1; i >= 0; i--){
    if(peterson_acquire(my_locks[i], my_roles[i]) < 0)
      return -1;
  }
  return 0;
}

int
tournament_release(void)
{
  for(int i = 0; i < L; i++){
    if(peterson_release(my_locks[i], my_roles[i]) < 0)
      return -1;
  }
  return 0;
}
