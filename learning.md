# What is Redis?
**in-memory key-value store** = super-fast dictionary that lives on a server and multiple programs can talk to over a network. Used for:
- **Caching** = saving work you already did (ex. query db once instead of 100 times)
- **Session Storage** = server doesn't remember who you are from one click to the next, so a session ID can be stored with your details to keep you logged in as you browse.
- **Leaderboards** = can return rank in near constant time
- **Message Queues** = drops task into redis queue and another worker processes that in the background
- **Rate Limiting** = prevent bots by only allowing so many requests
---
## What I am going to learn:
1. *Network Programming* 
    - Connect over **TCP** (reliable delivery service) so not just local program
    - How to programs talk to each other over a network using **sockets** (way to send and recieve data)
    - **Server** (listener) vs. **client** (initiator)
    - Custom Request-Response Protocol (like HTTP) = for talking between server and client
2. *Event Drive*
    - Not a new thread per connection (instead we create **event loop** to tell you when something is ready)
        - How Node.js (js outside browser), Nginx (forwards requests to the right application servers), and Redis (data store (key value)) work
    - **epoll** is like notifaction center so when they get data it tells you which sockets have data
        - We can have one thread and thousands of sockets (just a file descriptor)
3. *Data Structure*
    |Structure|Used for|
    |---------|--------|
    |**Hash table**|key-value store|
    |Balanced Binary Tree **AVL**|Sorted sets (range queries)|
    |Heap/timers|TTL/cache expiration|
    |Thread Pools|Background Tasks|
    ---
    ### Table explained:
    - AVL = difference in height between left and right subtree can be at most 1
        - Good for searching between ranges (like scores between 100 and 200)
    - Sorted Sets = combines hash map (for fast lookups) and balanced tree to keep sorted by score
    - Heaps = smallest (or largest) is always at the top
    - TTL/cache expiration = Time to live (it is a mechanism to delete keys after certain time)
        - Cache would grow forever without it 
    - Thread pool = pre-create a fixed number of threads and when a task comes in it goes to an idle thread
    - Background Task = actual work being done by thread pool