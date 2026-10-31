#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("Usage: %s <seed> <array_size>\n", argv[0]);
    return 1;
  }

  const char *seed       = argv[1];
  const char *array_size = argv[2];

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    return 1;
  }

  if (pid == 0) {
    // child: replace image with sequential_min_max
    execl("./sequential_min_max", "sequential_min_max",
          seed, array_size, (char *)NULL);
    // if execl returns — error
    perror("execl");
    _exit(127);
  }

  // parent: wait for child
  int status = 0;
  while (waitpid(pid, &status, 0) < 0) {
    if (errno == EINTR) continue;
    perror("waitpid");
    return 1;
  }

  return 0;
}