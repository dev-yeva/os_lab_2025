#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

pid_t *g_children_pids = NULL;
int g_pnum = 0;

// Обработчик сигнала SIGALRM срабатывает по истечении timeout
void sigalrm_handler(int sig) {
  if (sig == SIGALRM) {
    printf("Timeout reached! Killing child processes...\n");
    if (g_children_pids != NULL) {
      for (int i = 0; i < g_pnum; i++) {
        if (g_children_pids[i] > 0) {
          kill(g_children_pids[i], SIGKILL);
        }
      }
    }
  }
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;
  int timeout = -1; // Значение таймаута по умолчанию (-1 — не задан)

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"timeout", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
              printf("seed must be positive\n");
              return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
              printf("array_size must be positive\n");
              return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
              printf("pnum must be positive\n");
              return 1;
            }
            break;
          case 3:
            timeout = atoi(optarg);
            if (timeout <= 0) {
              printf("timeout must be positive\n");
              return 1;
            }
            break;
          case 4:
            with_files = true;
            break;

          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"num\"]\n",
           argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = pnum;

  // массив PID-ов детей для обработчика сигнала
  g_pnum = pnum;
  g_children_pids = malloc(sizeof(pid_t) * pnum);

  int pipes[2 * pnum];

  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      if (pipe(&pipes[2 * i]) < 0) {
        printf("Pipe creation failed!\n");
        free(array);
        free(g_children_pids);
        return 1;
      }
    }
  }

  // Если задан timeout, установка обработчика SIGALRM и запускаем alarm
  if (timeout > 0) {
    if (signal(SIGALRM, sigalrm_handler) == SIG_ERR) {
      perror("signal failed");
      free(array);
      free(g_children_pids);
      return 1;
    }
    alarm(timeout);
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      // successful fork
      if (child_pid == 0) {
        // child process
        // sleep(5); 

        unsigned int begin = (array_size * i) / pnum;
        unsigned int end   = (array_size * (i + 1)) / pnum;

        struct MinMax min_max = GetMinMax(array, begin, end);

        if (with_files) {
          char filename[64];
          snprintf(filename, sizeof(filename), "min_max_%d.txt", i);
          FILE *f = fopen(filename, "w");
          if (!f) {
            printf("Failed to open file for writing\n");
            exit(1);
          }
          fprintf(f, "%d %d\n", min_max.min, min_max.max);
          fclose(f);
        } else {
          for (int j = 0; j < pnum; j++) {
            if (j != i) {
              close(pipes[2 * j]);
              close(pipes[2 * j + 1]);
            }
          }
          close(pipes[2 * i]); // close read end in child
          write(pipes[2 * i + 1], &min_max, sizeof(min_max));
          close(pipes[2 * i + 1]);
        }
        free(array);
        free(g_children_pids);
        return 0;
      } else {
        g_children_pids[i] = child_pid;
      }

    } else {
      printf("Fork failed!\n");
      free(array);
      free(g_children_pids);
      return 1;
    }
  }

  // close write ends 
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      close(pipes[2 * i + 1]);
    }
  }

  // Неблокирующий цикл ожидания дочерних процессов с использованием waitpid и WNOHANG
  while (active_child_processes > 0) {
    pid_t status_pid = waitpid(-1, NULL, WNOHANG);
    if (status_pid > 0) {
      active_child_processes -= 1;
    } else if (status_pid == -1) {
      break;
    } else {
      // status_pid == 0: ни один процесс еще не завершился
      // пауза, чтобы не нагружать процессор постоянным опросом
      usleep(10000); 
    }
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      char filename[64];
      snprintf(filename, sizeof(filename), "min_max_%d.txt", i);
      FILE *f = fopen(filename, "r");
      if (!f) {
        printf("Failed to read from file: %s (maybe killed by timeout)\n", filename);
        continue; // Пропускаем недописанные/удаленные файлы
      }
      if (fscanf(f, "%d %d", &min, &max) != 2) {
        printf("Failed to read from file: %s\n", filename);
        fclose(f);
        continue;
      }
      fclose(f);
      remove(filename); // cleanup
    } else {
      struct MinMax child_mm;
      ssize_t r = read(pipes[2 * i], &child_mm, sizeof(child_mm));
      close(pipes[2 * i]);
      if (r != sizeof(child_mm)) {
        // Процесс мог быть убит по таймауту до того, как записал данные в пайп
        continue; 
      }
      min = child_mm.min;
      max = child_mm.max;
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  free(g_children_pids);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}