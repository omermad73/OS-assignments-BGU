#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: tournament N\n");
    exit(1);
  }
  int N = atoi(argv[1]);
  int tid = tournament_create(N);
  if (tid < 0) {
    printf("create failed\n");
    exit(1);
  }
  tournament_acquire();
  printf("PID %d, index %d acquired root\n", getpid(), tid);
  tournament_release();
  exit(0);
}
