#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>

/* TCP is a byte stream. Our protocol uses one newline-terminated line. */
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

int main(int argc, char **argv)
{
    int fd = -1, status = EXIT_FAILURE;
    struct sockaddr_in addr = {0};
    char buf[1024], *end;
    ssize_t n;

    /* 使い方: ./client IP PORT A B */
    if (argc != 5) {
        fprintf(stderr, "usage: %s IPv4 PORT A B\n", argv[0]);
        return EXIT_FAILURE;
    }

    errno = 0;
    long port = strtol(argv[2], &end, 10);
    if (errno || *end || end == argv[2] || port < 1 || port > 65535) {
        fprintf(stderr, "invalid port\n");
        return EXIT_FAILURE;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    if (inet_pton(AF_INET, argv[1], &addr.sin_addr) != 1) {
        fprintf(stderr, "invalid IPv4 address\n");
        return EXIT_FAILURE;
    }

    signal(SIGPIPE, SIG_IGN);

    /* STEP 1: socket */
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); goto done; }

    /* STEP 2: connect */
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        goto done;
    }

    /* STEP 3: ADDコマンドを作成して送信 */
    int a = atoi(argv[3]);
    int b = atoi(argv[4]);
    snprintf(buf, sizeof(buf), "ADD %d %d\n", a, b);

    if (write_all(fd, buf, strlen(buf)) < 0) {
        goto done;
    }

    /* STEP 4: サーバーからのレスポンスを受信して表示 */
    n = read_line(fd, buf, sizeof(buf));
    if (n < 0) {
        perror("read_line");
        goto done;
    }
    if (n == 0) {
        puts("server closed connection");
        goto done;
    }

    printf("server -> %s", buf);
    status = EXIT_SUCCESS;

done:
    /* STEP 5: close */
    if (fd >= 0) close(fd);
    return status;
}