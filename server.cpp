#include <iostream> // for input/output like cin and cout
#include <vector> // Dynamic arrays
#include <algorithm> // common algos like sort
#include <sys/socket.h> // For socket and the values we plug in
#include <netinet/in.h> // for sockaddr_in
#include <cstdio> // for perror()
#include <cstdlib> // for exit()
#include <unistd.h>
#include <cstring> // for strlen()
#include <cstdint> // for int32_t
#include <cassert>

void die(const char *message) {
    perror(message);
    exit(1); // kill the program
}

// will read 1 request and write one response
static int32_t read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            // it would return exactly 0 when EOF, client closed connection
            // it would be -1 when there is a network error like router crashed
            return -1; // error as rv should be reading at least one byte
        }
        assert((size_t)rv <= n); // this is guaranteed though for no fail
        n -= (size_t)rv;
        buf += rv; // move it forward so that OS can write to memory that 
        // hasn't already been written to
    }
    return 0; // all good, loop is done
}

static int32_t write_all(int fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            // -1 would be like if client disconnected
            // 0 would be impossible
            return -1; // for error
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
}

int main() {
    // Sets up IPv4 and TCP:
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    // Connection has not been created, we set up additional configuration
    int val = 1; // this is the value we are going to set it too
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    // SO_REUSEADDR for socket option reuse address (it is like
    // the override and says it is fine if there are packets leftover
    // we want to reset it anyways, like as it sometimes reserves 
    // the address a bit after in case any packets are still coming in)
    // It will be in TIME_WAIT, and we are overriding this
    // We don't want it to complain about the port being in use
    struct sockaddr_in addr = {}; // 0's every byte
    addr.sin_family = AF_INET; // interpret struct as IPv4
    addr.sin_port = htons(1234); // port, it converts host to network short
    // This is to store them as little endian for the cpu
    addr.sin_addr.s_addr = htonl(0); // Wildcard IP 0.0.0.0 so that it can
    // accept any requests coming into the machine
    int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
    // rv is 0 for success, 1 for failure that it was not able to bind it
    if (rv) { die("bind()"); }
    // socket is actually created after doing listen as that is when it will
    // start collecting responses that come to it
    // listen:
    // we are telling the operator system to open a queue for incoming 
    // network traffic
    // SOMAXCONN = Socket Maximum connections (4096, it is the biggest size
    // for the queue that we are able to safely handle)
    rv = listen(fd, SOMAXCONN);
    // loop that processes and accepts each client connection
    while (true) {
        // accept:
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);
        // we give the address so we tell it the max size, but then accept
        // will also modify addrlen to say how much it wrote
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
        // client_addr has the peer address
        if (connfd < 0) {
            continue;
        }
        // we are going to keep talking to the client till they hang up:
        // so we can handle multiple requests in a single connection
        while (true) {
            int32_t err = one_request(connfd);
            if (err) {
                break; // we handled all the multiple requests
            }
        }
        close(connfd);
    }
    return 0;
}