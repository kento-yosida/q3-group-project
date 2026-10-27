#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

/* TCP is a byte stream. Our protocol uses one newline-terminated line.
   One-byte reads keep this helper easy to inspect, not optimized. */
static ssize_t read_line(int fd, char *buf, size_t capacity)
{
    size_t used = 0;
    while (used + 1 < capacity) {
        ssize_t n = read(fd, buf + used, 1);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) {
            if (used == 0) return 0;
            errno = EPROTO; /* incomplete line */
            return -1;
        }
        if (buf[used++] == '\n') {
            buf[used] = '\0';
            return (ssize_t)used;
        }
    }
    errno = EMSGSIZE;
    return -1;
}

/* write() can send fewer bytes than requested. */
static int write_all(int fd, const char *buf, size_t length)
{
    size_t sent = 0;
    while (sent < length) {
        ssize_t n = write(fd, buf + sent, length - sent);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("write");
            return -1;
        }
        if (n == 0) { errno = EIO; perror("write"); return -1; }
        sent += (size_t)n;
    }
    return 0;
}

static int handle_client(int client_fd)
{
    char buf[1024];
    ssize_t n;

    while ((n = read_line(client_fd, buf, sizeof(buf))) > 0) {
        printf("client -> %s", buf);
        fflush(stdout);

        char cmd[32];
        char response[128];
        int a, b;

        if (sscanf(buf, "%31s", cmd) != 1) {
            snprintf(response, sizeof(response),
                     "ERROR unknown-command\n");

        } else if (strcmp(cmd, "ADD") == 0) {
            if (sscanf(buf, "ADD %d %d", &a, &b) == 2) {
                snprintf(response, sizeof(response), "OK %d\n", a + b);
            } else {
                snprintf(response, sizeof(response),
                         "ERROR invalid-argument\n");
            }

        } else if (strcmp(cmd, "SUB") == 0) {
            if (sscanf(buf, "SUB %d %d", &a, &b) == 2) {
                snprintf(response, sizeof(response), "OK %d\n", a - b);
            } else {
                snprintf(response, sizeof(response),
                         "ERROR invalid-argument\n");
            }

        } else if (strcmp(cmd, "MUL") == 0) {
            if (sscanf(buf, "MUL %d %d", &a, &b) == 2) {
                snprintf(response, sizeof(response), "OK %d\n", a * b);
            } else {
                snprintf(response, sizeof(response),
                         "ERROR invalid-argument\n");
            }

        } else {
            snprintf(response, sizeof(response),
                     "ERROR unknown-command\n");
        }

        if (write_all(client_fd, response, strlen(response)) < 0) {
            return -1;
        }
    }

    if (n < 0) {
        perror("read_line");
        return -1;
    }

    puts("connection closed");
    return 0;
}

static void *client_worker(void *arg)
{
    int client_fd = *(int *)arg;
    free(arg);

    handle_client(client_fd);

    close(client_fd);
    return NULL;
}

int main(void)
{
    int listen_fd = -1;
    int status = EXIT_FAILURE;
    struct sockaddr_in addr = {0};

    signal(SIGPIPE, SIG_IGN);

    /* STEP 1: socket */
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        goto done;
    }

    int yes = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR,
                   &yes, sizeof(yes)) < 0) {
        perror("setsockopt");
        goto done;
    }

    /* STEP 2: bind */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        goto done;
    }

    /* STEP 3: listen */
    if (listen(listen_fd, 5) < 0) {
        perror("listen");
        goto done;
    }

    puts("LISTEN 0.0.0.0:5000");
    fflush(stdout);

    /* STEP 4: accept and create threads */
    status = EXIT_SUCCESS;

    for (;;) {
        int client_fd = accept(listen_fd, NULL, NULL);

        if (client_fd < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            status = EXIT_FAILURE;
            break;
        }

        printf("connected: client_fd=%d\n", client_fd);
        fflush(stdout);

        /* 接続ごとに専用のメモリを確保 */
        int *arg = malloc(sizeof(int));

        if (arg == NULL) {
            perror("malloc");
            close(client_fd);
            continue;
        }

        *arg = client_fd;

        /* 新しいスレッドを作成 */
        pthread_t tid;
        int rc = pthread_create(&tid, NULL, client_worker, arg);

        if (rc != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(rc));
            free(arg);
            close(client_fd);
            continue;
        }

        /* 終了したスレッドの資源を自動回収 */
        rc = pthread_detach(tid);
        if (rc != 0) {
            fprintf(stderr, "pthread_detach: %s\n", strerror(rc));
            pthread_join(tid, NULL);
        }
    }

done:
    if (listen_fd >= 0) close(listen_fd);
    return status;
}