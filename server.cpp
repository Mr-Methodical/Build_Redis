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
    int flags = fcntl(fd, F_GETFL, 0); // gets the current flags
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

// accepting the connection that is knocking at our door
static Conn *handle_accept(int fd) {
    // accept:
    struct sockaddr_in client_addr = {}; // for IPv4
    socklen_t addrlen = sizeof(client_addr);
    int conn_fd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
    if (conn_fd < 0) {
        return NULL;
    }
    // set the new connection to non-blocking mode:
    fd_set_nb(conn_fd);
    Conn *conn = new Conn();
    conn->fd = conn_fd;
    conn->want_read = true; // read what the connecting person is telling us
    return conn;
}

// append to the back:
static void
buf_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len) {
    buf.insert(buf.end(), data, data + len);
}

// remove from the front:
static void buf_consume(std::vector<uint8_t> &buf, size_t n) {
    buf.erase(buf.begin(), buf.begin() + n);
}

// if there is enough data then we will process the request
static bool try_one_request(Conn *conn) {
    // try to parse the accumulated buffer
    // Our protocol is that the message header is the first 4 bytes, and it
    // tells us how long our message is going to be
    if (conn->incoming.size() < 4) {
        return false; // we need to read more instead
    }
    uint32_t len = 0;
    memcpy(&len, conn->incoming.data(), 4);
    if (len > k_max_msg) {
        // this is a protocol error because we can only take so big
        conn->want_close = true;
        return false;
    }
    // protocol: message body
    if (4 + len > conn->incoming.size()) {
        return false; // we actually want to read the rest of the message
        // like we haven't been given the full message yet
    }
    const uint8_t *request = &conn->incoming[4]; // address to message
    // Process the parsed message
    // ...
    // Generate the response (echo)
    buf_append(conn->outgoing, (const uint8_t *)&len, 4);
    buf_append(conn->outgoing, request, len);
    // remove the message from conn::incoming
    buf_consume(conn->incoming, 4 + len);
    return true; // we were able to read it
}

static void handle_read(Conn *conn) {
    // non-blocking read:
    uint8_t buf[64 * 1024];
    ssize_t rv = read(conn->fd, buf, sizeof(buf));
    if (rv <= 0) {
        // case of IO error or EOF where rv == 0
        conn->want_close = true;
        return;
    }
    // add new data to the conn->incoming buffer
    buf_append(conn->incoming, buf, (size_t)rv);
    // try to parse the accumulated buffer
    // Process the parsed message
    // remove the message from from conn->incoming
    try_one_request(conn);
    // update the readiness intention
    if (conn->outgoing.size() > 0) {
        // we have a response that we want to give like to send back
        conn->want_read = false;
        conn->want_write = true;
    } // else we want to keep reading what they client wants to say
}

// non-blocking write:
static void handle_write(Conn *conn) {
    assert(conn->outgoing.size() > 0);
    ssize_t rv = write(conn->fd, conn->outgoing.data(), conn->outgoing.size());
    if (rv < 0) {
        conn->want_close = true; // like if not able to write
        return;
    }
    // remove the writtend data from the outgoing
    buf_consume(conn->outgoing, (size_t)rv);
    // update the readiness intention so we can start recieving messages again
    if (conn->outgoing.size() == 0) {
        // all data has been sent out, now we can read what they respond with
        conn->want_read = true;
        conn->want_write = false;
    } // else we will just still want to write
}

const size_t k_max_msg = 32 << 20; // 32 megabytes is the most we will allow

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

    // a flat array where index matches the client's fd
    std::vector<Conn *> fd2conn; 
    // checklist that we will hand to OS:
    std::vector<struct pollfd> poll_args;
    // pollfd has 3 fields: fd, events to tell the OS "wake me up when I am
    // allowed to read/write", and revents is what is actually safe to do right 
    // now
    // the event loop:
    while (true) {
        // clear from the previous iteration
        poll_args.clear();

        // Note: POLLIN for listening socket means that it is checking for 
        // new connections, but POLLIN for a connection means that we are
        // pulling in data from that client

        // Main listening socket on checklist first to accept new connections
        struct pollfd pfd = {fd, POLLIN, 0};
        poll_args.push_back(pfd);
        // we are consturcting the fd list for poll();
        for (Conn *conn : fd2conn) {
            if (!conn) {
                continue; // as there is not connection here
            }
            struct pollfd pfd = {conn->fd, POLLERR, 0};
            if (conn->want_read) {
                pfd.events |= POLLIN;
            }
            if (conn->want_write) {
                pfd.events |= POLLOUT;
            }
            poll_args.push_back(pfd); // this is task we want to do this turn
        }
        
        // now we are waiting for readiness
        // data method is for giving a pointer to the first element
        int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), -1);
        // poll is the only blocking call, only wakes up when one of fds ready
        if (rv < 0 && errno == EINTR) {
            // this is a signal like resizing terminal window or background
            // timer, when signal hits it triggers poll
            continue;
        }
        // real error we return
        if (rv < 0) {
            die("poll()");
        }

        // handle the listening socket
        if (poll_args[0].revents) {
            if (Conn *conn = handle_accept(fd)) {
                // put into the map
                if (fd2conn.size() <= (size_t)conn->fd) {
                    fd2conn.resize(conn->fd + 1);
                }
                fd2conn[conn->fd] = conn;
            }
        }

        // now handle connection sockets:
        for (size_t i = 1; i < poll_args.size(); ++i) {
            uint32_t ready = poll_args[i].revents;
            Conn *conn = fd2conn[poll_args[i].fd];
            if (ready & POLLIN) { // the POLLIN is all 0 except for 
                // index where it is on so we are testing if that index is on
                // for ready
                handle_read(conn); // this is just the application logic
            }
            if (ready & POLLOUT) {
                handle_write(conn); // more applciation logic we need to write
            }
            // ready & POLLERR if OS noticed they pulled the plug on the 
            // connection, and conn->want_close was that our application decided
            // that we wanted to close the connection
            if ((ready & POLLERR) || conn->want_close) {
                (void)close(conn->fd); // we don't care about the return value
                fd2conn[conn->fd] = NULL;
                delete conn;
            }
        }
    }
}