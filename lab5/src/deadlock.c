#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;

// Поток 1: захватывает m1, потом m2
void *thread1(void *arg) {
    (void)arg;
    pthread_mutex_lock(&m1);
    printf("T1: захватил m1, жду m2...\n");
    sleep(1);                       // для T2 время захватить m2
    pthread_mutex_lock(&m2);     
    printf("T1: захватил m2\n");
    pthread_mutex_unlock(&m2);
    pthread_mutex_unlock(&m1);
    return NULL;
}

// Поток 2: захватывает m2, потом m1  (обратный порядок!)
void *thread2(void *arg) {
    (void)arg;
    pthread_mutex_lock(&m2);
    printf("T2: захватил m2, жду m1...\n");
    sleep(1);
    pthread_mutex_lock(&m1);  
    printf("T2: захватил m1\n");
    pthread_mutex_unlock(&m1);
    pthread_mutex_unlock(&m2);
    return NULL;
}

int main(void) {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, thread1, NULL);
    pthread_create(&t2, NULL, thread2, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("Никогда не напечатается\n");
    return 0;
}