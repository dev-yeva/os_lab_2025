#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <getopt.h>
#include <pthread.h>

#include "utils.h"
#include "sum.h"

int main(int argc, char **argv) {
  uint32_t threads_num = 0;
  uint32_t array_size = 0;
  uint32_t seed = 0;

  while (true) {
    static struct option options[] = {
      {"threads_num", required_argument, 0, 0},
      {"seed", required_argument, 0, 0},
      {"array_size", required_argument, 0, 0},
      {0, 0, 0, 0}
    };

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            threads_num = atoi(optarg);
            if (threads_num <= 0) {
              printf("threads_num must be positive\n");
              return 1;
            }
            break;
          case 1:
            seed = atoi(optarg);
            if (seed <= 0) {
              printf("seed must be positive\n");
              return 1;
            }
            break;
          case 2:
            array_size = atoi(optarg);
            if (array_size <= 0) {
              printf("array_size must be positive\n");
              return 1;
            }
            break;
          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (threads_num == 0 || array_size == 0 || seed == 0) {
    printf("Usage: %s --threads_num \"num\" --seed \"num\" --array_size \"num\"\n", argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  // Генерация массива не должна попадать в замер времени
  GenerateArray(array, array_size, seed);

  pthread_t threads[threads_num];
  struct SumArgs args[threads_num];

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  for (uint32_t i = 0; i < threads_num; i++) {
    args[i].array = array;
    args[i].begin = (array_size * i) / threads_num;
    args[i].end = (array_size * (i + 1)) / threads_num;

    if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i])) {
      printf("Error: pthread_create failed!\n");
      free(array);
      return 1;
    }
  }

  int total_sum = 0;
  for (uint32_t i = 0; i < threads_num; i++) {
    int sum = 0;
    pthread_join(threads[i], (void **)&sum);
    total_sum += sum;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);

  printf("Total: %d\n", total_sum);
  printf("Elapsed time: %fms\n", elapsed_time);
  return 0;
}