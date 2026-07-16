#include <iostream> // for input/output like cin and cout
#include <vector> // Dynamic arrays
#include <algorithm> // common algos like sort
#include <sys/socket.h> // For socket and the values we plug in
#include <netinet/in.h> // for sockaddr_in
#include <cstdio> // for perror()
#include <cstdlib> // for exit()

void die(const char *message) {
    perror(message);
    exit(1); // kill the program
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
    return 0;
}