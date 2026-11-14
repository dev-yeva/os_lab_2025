
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>

#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "pthread.h"
#include "common.h"

struct Server {
    char ip[255];
    int port;
};

struct ServerArgs {
    struct Server server;
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
    uint64_t result;
    bool success;
};


bool ConvertStringToUI64(const char *str, uint64_t *val) {
    char *end = NULL;

    errno = 0;
    unsigned long long i = strtoull(str, &end, 10);

    if (errno == ERANGE || end == str || *end != '\0') {
        fprintf(stderr, "Invalid uint64_t value: %s\n", str);
        return false;
    }

    *val = i;
    return true;
}

bool SendAll(int sck, const char *buffer, size_t size) {
    size_t sent = 0;

    while (sent < size) {
        ssize_t n = send(sck, buffer + sent, size - sent, 0);

        if (n <= 0)
            return false;

        sent += n;
    }

    return true;
}

bool RecvAll(int sck, char *buffer, size_t size) {
    size_t received = 0;

    while (received < size) {
        ssize_t n = recv(sck, buffer + received, size - received, 0);

        if (n <= 0)
            return false;

        received += n;
    }

    return true;
}

void *ThreadServer(void *arg) {
    struct ServerArgs *args = (struct ServerArgs *)arg;

    struct addrinfo hints;
    struct addrinfo *result = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    char port[16];
    snprintf(port, sizeof(port), "%d", args->server.port);

    int err = getaddrinfo(args->server.ip, port, &hints, &result);

    if (err != 0) {
        fprintf(stderr, "getaddrinfo failed: %s\n",
                gai_strerror(err));
        return NULL;
    }

    int sck = socket(result->ai_family,
                     result->ai_socktype,
                     result->ai_protocol);

    if (sck < 0) {
        perror("Socket creation failed");
        freeaddrinfo(result);
        return NULL;
    }

    if (connect(sck, result->ai_addr, result->ai_addrlen) < 0) {
        perror("Connection failed");
        close(sck);
        freeaddrinfo(result);
        return NULL;
    }

    freeaddrinfo(result);

    char task[sizeof(uint64_t) * 3];

    memcpy(task, &args->begin, sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t), &args->end, sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t), &args->mod, sizeof(uint64_t));

    if (!SendAll(sck, task, sizeof(task))) {
        fprintf(stderr, "Send failed\n");
        close(sck);
        return NULL;
    }

    char response[sizeof(uint64_t)];

    if (!RecvAll(sck, response, sizeof(response))) {
        fprintf(stderr, "Receive failed\n");
        close(sck);
        return NULL;
    }

    memcpy(&args->result, response, sizeof(uint64_t));
    args->success = true;

    printf("Server %s:%d calculated [%llu, %llu]: %llu\n",
           args->server.ip,
           args->server.port,
           (unsigned long long)args->begin,
           (unsigned long long)args->end,
           (unsigned long long)args->result);

    close(sck);

    return NULL;
}

int main(int argc, char **argv) {
    uint64_t k = -1;
    uint64_t mod = -1;
    char servers[255] = {'\0'};

    while (true) {
        static struct option options[] = {
            {"k", required_argument, 0, 0},
            {"mod", required_argument, 0, 0},
            {"servers", required_argument, 0, 0},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case 0:
            switch (option_index) {
            case 0:
                if (!ConvertStringToUI64(optarg, &k))
                    return 1;
                break;

            case 1:
                if (!ConvertStringToUI64(optarg, &mod))
                    return 1;
                break;

            case 2:
                if (strlen(optarg) >= sizeof(servers)) {
                    fprintf(stderr, "Servers file path is too long\n");
                    return 1;
                }

                strcpy(servers, optarg);
                break;

            default:
                printf("Index %d is out of options\n", option_index);
            }
            break;

        case '?':
            printf("Arguments error\n");
            return 1;

        default:
            fprintf(stderr, "getopt returned character code 0%o?\n", c);
        }
    }

    if (k == UINT64_MAX || mod == UINT64_MAX || !strlen(servers) || mod == 0) {
        fprintf(stderr,
                "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
                argv[0]);
        return 1;
    }

    FILE *file = fopen(servers, "r");

    if (file == NULL) {
        perror("fopen");
        return 1;
    }

    unsigned int servers_num = 0;
    struct Server *to = NULL;

    char line[512];

    while (fgets(line, sizeof(line), file)) {
        struct Server server;

        if (sscanf(line, "%254[^:]:%d", server.ip, &server.port) != 2)
            continue;

        if (server.port < 1 || server.port > 65535) {
            fprintf(stderr, "Invalid port: %d\n", server.port);
            continue;
        }

        struct Server *tmp = realloc(
            to, (servers_num + 1) * sizeof(struct Server)
        );

        if (tmp == NULL) {
            perror("realloc");
            free(to);
            fclose(file);
            return 1;
        }

        to = tmp;
        to[servers_num++] = server;
    }

    fclose(file);

    if (servers_num == 0) {
        fprintf(stderr, "No servers found\n");
        free(to);
        return 1;
    }

    if (k == 0) {
        printf("answer: %llu\n",
               (unsigned long long)(1 % mod));
        free(to);
        return 0;
    }

    if (k < servers_num)
        servers_num = (unsigned int)k;

    pthread_t *threads = malloc(sizeof(pthread_t) * servers_num);
    struct ServerArgs *args = calloc(servers_num, sizeof(struct ServerArgs));

    if (threads == NULL || args == NULL) {
        perror("malloc");
        free(threads);
        free(args);
        free(to);
        return 1;
    }

    uint64_t range = k;
    uint64_t chunk = range / servers_num;
    uint64_t remainder = range % servers_num;

    uint64_t current = 1;

    unsigned int created = 0;

    for (unsigned int i = 0; i < servers_num; i++) {
        uint64_t size = chunk + (i < remainder);

        args[i].server = to[i];
        args[i].begin = current;
        args[i].end = current + size - 1;
        args[i].mod = mod;

        current += size;

        if (pthread_create(&threads[i], NULL, ThreadServer, &args[i])) {
            fprintf(stderr, "pthread_create failed\n");
            break;
        }

        created++;
    }

    uint64_t answer = 1;
    bool success = created == servers_num;

    for (unsigned int i = 0; i < created; i++) {
        pthread_join(threads[i], NULL);

        if (!args[i].success) {
            success = false;
            continue;
        }

        answer = MultModulo(answer, args[i].result, mod);
    }

    if (success) {
        printf("answer: %llu\n", (unsigned long long)answer);
    } else {
        fprintf(stderr, "Some server calculations failed\n");
    }

    free(threads);
    free(args);
    free(to);

    return success ? 0 : 1;
}
