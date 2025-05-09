#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static int N;               // number of processes
static int L;               // number of levels
static int *all_locks;      // global locks array of size N-1
static int *my_locks;       // per-process lock IDs array of size L
static int *my_roles;       // per-process roles array of size L
static int my_index;        // process index

int
node_index(int level, int idx)
{
  // BFS order
  return ((1 << level) - 1) + (idx >> (L - level));
}

int
compute_role(int level, int idx)
{
  return (idx & (1 << (L - level - 1))) >> (L - level - 1);
}

int
tournament_create(int processes)
{
  //integrity check
  if(processes <= 0 || processes > 16)
      return -1;

   //check if processes is power of 2
  if (processes & (processes - 1))
      return -1;

  N = processes;
  L = 0;
  int temp = processes;
  // computing the number of levels
  while(temp > 1) {
    temp = temp >> 1;
    L++;
  }

  // allocate global locks array
  all_locks = malloc(sizeof(int) * (N - 1));
  // allocate locks fot each process
  my_locks = malloc(sizeof(int)*L);
  // allocate roles fot each process
  my_roles = malloc(sizeof(int)*L);
  if (!all_locks || !my_locks || !my_roles)
    return -1;

  //create peterson locks   
  for (int i = 0; i < N - 1; i++) {
      all_locks[i] = peterson_create();
      if (all_locks[i] < 0)
          return -1;
  }

  //create processes and giving each of them unigue index
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

  // assighning for each process its locks and roles 
  //  for each level
  for (int l = 0; l < L; l++) {
      int idx = node_index(l, my_index);
      my_locks[l] = all_locks[idx];
      my_roles[l] = compute_role(l, my_index);
  }

  return my_index;
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
