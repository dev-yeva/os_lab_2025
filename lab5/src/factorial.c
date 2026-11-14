#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

long long result = 1;
long long mod;
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

typedef struct { long long from, to; } Arg;

void *worker(void *p) {
    Arg *a = (Arg *)p;
    long long part = 1;
    for (long long i = a->from; i <= a->to; i++)
        part = (part * (i % mod)) % mod;
    pthread_mutex_lock(&mut);
    result = (result * part) % mod;
    pthread_mutex_unlock(&mut);
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 5) {
        printf("Usage: %s -k <k> --pnum=<n> --mod=<m>\n", argv[0]);
        return 1;
    }

    long long k   = atoll(argv[2]);        
    int       pnum = atoi(argv[3] + 7);   
    mod            = atoll(argv[4] + 6);  

    if (k < 0 || pnum < 1 || mod == 0) {
        printf("Bad arguments\n");
        return 1;
    }
    if (mod < 0) mod = -mod;
    if (mod == 1 || k == 0) {
        printf("%lld! mod %lld = %lld\n", k, mod, 1 % mod);
        return 0;
    }

    pthread_t tids[pnum];
    Arg       args[pnum];

    long long size = k / pnum;
    long long rem  = k % pnum;
    long long next = 1;

    for (int i = 0; i < pnum; i++) {
        long long cnt = size + (i < rem ? 1 : 0);
        args[i].from = next;
        args[i].to   = next + cnt - 1;
        next += cnt;
        pthread_create(&tids[i], NULL, worker, &args[i]);
    }
    for (int i = 0; i < pnum; i++)
        pthread_join(tids[i], NULL);

    printf("%lld! mod %lld = %lld\n", k, mod, result);
    return 0;
}