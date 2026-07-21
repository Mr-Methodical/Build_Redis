# Build Redis

A Redis-compatible in-memory key-value server, written from scratch in C++ on Linux.
No networking frameworks, no external dependencies — raw POSIX sockets up.

The goal is to understand how a real database server works end to end: how bytes
move across a socket, how a protocol is framed, how one thread serves thousands of
clients, and how the underlying data structures are actually built.

## Status

**Part 1 — core server (in progress)**

- [x] TCP socket server and client
- [x] `read_full` / `write_all` — correct handling of partial reads and writes
- [x] `EINTR` handling so syscalls resume instead of failing
- [x] Length-prefixed request-response protocol
- [x] RAII resource management so sockets close on every exit path
- [ ] Non-blocking event loop (in progress)
- [ ] Key-value command handling (`GET` / `SET` / `DEL`)

**Part 2 — planned**

- [ ] Custom chained hashtable with progressive rehashing
- [ ] Sorted set backed by an AVL tree
- [ ] Timers and idle-connection timeouts
- [ ] `O(1)` TTL cache expiration via a min-heap
- [ ] Thread pool for expensive operations

## Build and run

```bash
g++ -Wall -Wextra -O2 -o server server.cpp
g++ -Wall -Wextra -O2 -o client client.cpp

./server        # terminal 1
./client        # terminal 2
```

## Notes

Some things that were more subtle than expected:

- **TCP is a byte stream, not a message stream.** One `write()` does not equal one
  `read()`, which is why the protocol needs explicit length prefixes and why
  `read_full`/`write_all` exist.
- **Byte order matters on the wire.** Integers are converted to big-endian
  (`htonl`/`ntohl`) before being sent.
- **Syscalls can be interrupted.** `read()` returning `-1` with `errno == EINTR`
  isn't a failure; the read has to be retried.

Following [build-your-own.org/redis](https://build-your-own.org/redis/), with all
code written by hand.
