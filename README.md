# Build Redis

A Redis-compatible in-memory key-value server, written from scratch in C++ on Linux.
No networking frameworks, no external dependencies — raw POSIX sockets up.

The goal is to understand how a real database server works end to end: how bytes
move across a socket, how a protocol is framed, how one thread serves thousands of
clients, and how the underlying data structures are actually built.

Following [build-your-own.org/redis](https://build-your-own.org/redis/), with all
code written by hand.

## Status

**Implemented**

- [x] TCP socket server and client over raw POSIX sockets
- [x] Length-prefixed request/response framing (4-byte length header)
- [x] `read_full` (with `EINTR` retry so interrupted reads resume instead of
      failing) and `write_all` — correct handling of partial reads and writes
- [x] RAII socket cleanup so descriptors close on every exit path
- [x] Non-blocking sockets (`O_NONBLOCK`) and a `poll()`-based event loop that
      accepts and tracks multiple connections in one thread

**In progress**

- [ ] Wiring the per-connection read/write handlers into the event loop so it
      processes messages end to end (the loop and connection state exist; the
      `handle_read` / `handle_write` bodies are still being written)
- [ ] `GET` / `SET` / `DEL` command handling
- [ ] Custom chained hashtable for the keyspace
- [ ] `O(1)` TTL expiration via a min-heap

**Later**

- [ ] Sorted set backed by an AVL tree
- [ ] Idle-connection timeouts
- [ ] Thread pool for expensive operations

## Layout

```
server.cpp          the event-loop server (single-threaded, poll()-based)
client.cpp          TCP client — length-prefixed protocol, read_full/write_all, RAII
prac/               scratch/practice implementations while learning the concepts
learning.md         running notes
demo/flappy-goose/  browser-game demo client (see below)
```

## Build and run

```bash
# client — builds and runs today
g++ -Wall -Wextra -O2 -o client client.cpp
./client

# server — event loop is mid-refactor (per-connection handlers are still being
# wired in), so it is not yet runnable end to end
g++ -Wall -Wextra -O2 -o server server.cpp
```

## Notes

Some things that were more subtle than expected:

- **TCP is a byte stream, not a message stream.** One `write()` does not equal one
  `read()`, which is why the protocol needs explicit length prefixes and why
  `read_full` / `write_all` exist.
- **Byte order.** The socket address and port go out in network byte order
  (`htonl` / `htons`). The message length prefix is currently written in native
  byte order — a known simplification (flagged in `client.cpp`) to revisit before
  this talks to a peer on a different-endian host.
- **Syscalls can be interrupted.** `read()` returning `-1` with `errno == EINTR`
  isn't a failure; `read_full` retries it, and the `poll()` loop handles `EINTR`
  the same way. (`write_all` currently handles partial writes but not `EINTR`.)

## Demo — Flappy Goose

[`demo/flappy-goose/`](demo/flappy-goose/) is a small HTML5-canvas browser game
used as a **demo client** for this project: it submits scores to a leaderboard
backend and renders a top-scores list. Once this server's command handling lands,
the plan is to point the game at this implementation. **Today it runs against a
stock Redis instance on an Azure VM**, not against this server yet.
