#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>

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

int main(void)
{
    int listen_fd = -1, client_fd = -1, status = EXIT_FAILURE;
    struct sockaddr_in addr = {0};
    char buf[1024];
    ssize_t n;
    signal(SIGPIPE, SIG_IGN);

    /* STEP 1: socket */
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) { perror("socket"); goto done; }
    int yes = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR,
                   &yes, sizeof(yes)) < 0) { perror("setsockopt"); goto done; }

    /* STEP 2: bind local address and port */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); goto done;
    }
    /* STEP 3: listen */
    if (listen(listen_fd, 5) < 0) { perror("listen"); goto done; }
    puts("LISTEN 0.0.0.0:5000"); fflush(stdout);

    /* STEP 4: accept returns a NEW descriptor. listen_fd remains open. */
    client_fd = accept(listen_fd, NULL, NULL);
    if (client_fd < 0) { perror("accept"); goto done; }
    printf("connected: listen_fd=%d client_fd=%d\n", listen_fd, client_fd);
    fflush(stdout);

    /* STEP 5: read/write using the connected descriptor */
    status = EXIT_SUCCESS;
    while ((n = read_line(client_fd, buf, sizeof(buf))) > 0) {
        printf("client -> %s", buf); fflush(stdout);
        /* ECHO変更箇所: replace the following two lines in class. */
        int a, b;
char response[128];

if (sscanf(buf, "ADD %d %d", &a, &b) == 2) {
    snprintf(response, sizeof(response), "OK %d\n", a + b);

} else if (sscanf(buf, "SUB %d %d", &a, &b) == 2) {
    snprintf(response, sizeof(response), "OK %d\n", a - b);

} else if (sscanf(buf, "MUL %d %d", &a, &b) == 2) {
    snprintf(response, sizeof(response), "OK %d\n", a * b);    

} else {
    snprintf(response, sizeof(response), "ERROR invalid-argument\n");
}

if (write_all(client_fd, response, strlen(response)) < 0) break;
    }
    if (n < 0) { perror("read_line"); status = EXIT_FAILURE; }
    puts("connection closed; restart server for the next client");

done:
    /* STEP 6: close each descriptor separately */
    if (client_fd >= 0) close(client_fd);
    if (listen_fd >= 0) close(listen_fd);
    return status;
}
