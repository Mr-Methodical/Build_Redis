#include <sys/socket.h>
#include <cstdlib> // for exit()
#include <cstdio> // for perror()
#include <netinet/in.h> // for sockaddr_in
#include <cstring> // for strlen
#include <unistd.h> // for write

void die(const char *message) {
    perror(message);
    exit(1); // kill the program
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET; // for the address family internet for IPv4
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // to use our own computer
    // 127.0.0.1
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("connect()");
    }
    char msg[] = "hello";
    write(fd, msg, strlen(msg));
    char rbuf[64] = {};
    ssize_t n = read(fd, rbuf, sizeof(rbuf) - 1);
    if (n < 0) {
        die("read()");
    }
    printf("server says: %s\n", rbuf);
    close(fd);
    return 0;
}