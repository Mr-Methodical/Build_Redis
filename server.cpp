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
    fprinf(stderr, "[%d] %s\n", errno, msg);
    abort(); // doesn't even clean up resources, just crashes immediately
}

// Helper to make a socket non-blocking:
static void fd_set_nb(int fd) {
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (errno)
}

const size_t k_max_msg = 4096;

using namespace std;

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
}