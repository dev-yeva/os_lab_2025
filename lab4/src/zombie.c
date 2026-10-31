#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    pid_t child_pid = fork();

    if (child_pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (child_pid == 0) {
        // Дочерний процесс сразу завершает работу
        printf("Child process (PID: %d) is exiting immediately.\n", getpid());
        exit(0);
    } else {
        // Родительский процесс выводит свой PID и засыпает на 30 секунд, 
        // НЕ вызывая wait(). В этот период дочерний процесс станет зомби.
        printf("Parent process (PID: %d) created child (PID: %d).\n", getpid(), child_pid);
        printf("Run 'ps aux | grep Z' in another terminal to see the zombie process.\n");
        
        sleep(60); // Пауза, во время которой можно наблюдать зомби

        // После сна родитель вызывает wait, чтобы убрать зомби
        printf("Parent waking up and cleaning up the child...\n");
        wait(NULL);
        printf("Zombie cleaned up. Exiting.\n");
    }

    return 0;
}