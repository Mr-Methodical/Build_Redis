#include <iostream> // for cout and cin
#include <vector>
#include <sys/socket.h> // like socket(), bind(), listen(), and accept()
#include <netinet/in.h> // network data structs like sockaddr and htons (endian)
#include <unistd.h> // POSIX operating system API like read(), write(), close()
#include <fcntl.h> // to set sockets to non-blocking (file control)
#include <poll.h> // for our event loop knowing when one is ready
#include <cstring> // memcpy()
#include <cstdint> // exact byte sizing like uint32_t
#include <cassert>
#include <errno.h> // error handling

static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

static void msg_errno(const char *msg) {
    fprintf(stderr, "[errno:%d] %s\n", errno, msg);
}

static void die(const char *msg) {
    fprintf(stderr, "[%d] %s\n", errno, msg);
    abort(); // doesn't even clean up resources, just crashes immediately
}

// Helper to make a socket non-blocking:
// because usually when we do accept(), read(), write() we will just completely
// freeze
static void fd_set_nb(int fd) {
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (errno) {
        die("fcntl error");
        return;
    }
    flags |= O_NONBLOCK;
    errno = 0;
    (void)fcntl(fd, F_SETFL, flags); // we are setting our fd with our flags
    if (errno) {
        die("fcntl error");
    }
}

const size_t k_max_msg = 32 << 20; // 32 megabytes is the most we will allow

// Our notebook for each client:
struct Conn {
    int fd = -1; // Clients table number that we refer to it as

    // Application intention
    bool want_read = false; // true if we want client to send us data
    bool want_write = false; // true if we have response to send back
    bool want_close = false; // true if there is an error, kick client

    // buffered input and output
    std::vector<uint8_t> incoming; // incomplete messages being read
    std::vector<uint8_t> outgoing; // responses that haven't fully sent yet
};

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET; // we use IPv4
    addr.sin_port = htons(1234); // convert from little endian to big endian
    // because network protocol is big endian and cpu is little endian
    addr.sin_addr.s_addr = htonl(0);
    int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("bind()");
    }
    rv = listen(fd, SOMAXCONN);
    if (rv) {
        die("listen()");
    }
    // make the main listening socket non-blocking
    fd_set_nb(fd);

    // checklist that we will hand to OS:
    std::vector<struct pollfd> poll_args;
    // pollfd has 3 fields: fd, events to tell the OS "wake me up when I am
    // allowed to read/write", and revents is what is actually safe to do right 
    // now
    while (true) {
        // clear from the previous iteration
        poll_args.clear();

        // Note: POLLIN for listening socket means that it is checking for 
        // new connections, but POLLIN for a connection means that we are
        // pulling in data from that client

        // Main listening socket on checklist first to accept new connections
        struct pollfd pfd = {fd, POLLIN, 0};
        poll_args.push_back(pfd);
        

    }
}