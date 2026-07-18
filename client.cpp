#include <sys/socket.h>
#include <cstdlib> // for exit()
#include <cstdio> // for perror()
#include <netinet/in.h> // for sockaddr_in
#include <cstring> // for strlen
#include <unistd.h> // for write
#include <cassert>
#include <errno.h>

const size_t k_max_msg = 4096;

static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

void die(const char *message) {
    perror(message);
    exit(1); // kill the program
}

// will read 1 request and write one response
static int32_t read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv < 0) {
            if (errno == EINTR) {
                // it was interupted system call so we can continue like normal
                continue;
            }
            return -1; // any other case we don't allow this
        }
        if (rv == 0) {
            return -1; // for the EOF
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
    return 0;
}

static int32_t query(int fd, const char *text) {
    // we are going to query the server
    uint32_t len = (uint32_t)strlen(text);
    if (len > k_max_msg) {
        return -1; // not possible
    }
    // send request:
    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4); // we are still assuming little endian here
    memcpy(&wbuf[4], text, len);
    if (int32_t err = write_all(fd, wbuf, 4 + len)) {
        // failed it didn't give success
        return err;
    }
    // 4 byte header:
    char rbuf[4 + k_max_msg];
    errno = 0;
    int32_t err = read_full(fd, rbuf, 4);
    if (err) {
        // there was some error but we are not sure if it was a good error
        // because hitting end of the file would not trigger it   
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }
    memcpy(&len, rbuf, 4);
    if (len > k_max_msg) {
        msg("too long");
        return -1; // to say it is an error
    }
    // reply body
    err = read_full(fd, &rbuf[4], len);
    if (err) {
        // something other than 0 which is not success
        msg("read() error");
    }
    // we will do something:
    printf("server says: %.*s\n", len, &rbuf[4]);
    return 0; // for no issues
}

struct SocketWrapper {
    int fd;
    // Constructor
    SocketWrapper(int file_descriptor) {
        fd = file_descriptor;
    }
    // Destructor automatically closes the object when it dies:
    ~SocketWrapper() {
        if (fd >= 0) {
            close(fd);
            fd = -1; // we mark it as closed
        }
    }
};

int main() {
    int raw_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (raw_fd < 0) {
        die("socket()");
    }
    // we will give this to my RAII to manage:
    SocketWrapper conn(raw_fd);

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET; // for the address family internet for IPv4
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // to use our own computer
    // 127.0.0.1
    int rv = connect(conn.fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("connect()");
    }
    int32_t err = query(conn.fd, "hello1");
    if (err) {
        // note that close(conn.fd) will be autocalled because of RAII
        return err;
    }
    err = query(conn.fd, "hello2");
    if (err) {
        return err;
    }
    return 0;
}