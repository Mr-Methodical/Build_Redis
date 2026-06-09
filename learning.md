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
---
## Chapter 1
- This will be mostly in C
- Final product will be around 1200 lines
- unordered_map is good when you want the fastest lookups and map for when you want range queries
- Redis is called a **Data structure server** (as it can map a string to hash, list, or sorted set)
- **in-memory** is RAM and storage is slower
- Redis is a **Cache** because it is meant to live entirely in RAM so it is fast
- It is stored in the server's RAM not the clients that is how it works (so when client requests it, it allows for them to get it back super quick)
    - This allows instead of the server that has the database, it can offload a lot of the work a Redis server
- list pagination - instead of listing everything at once we can give off a little bit information at a time and basically keep the index, not showing them like a million fields at once
- Caching servers are the easiest way to scale
---
## Chapter 2
- IP address is a unique number assigned to every device connected to network
    - IP address finds machine but a port finds the program on that machine
- TCP vs. UDP
    - TCP = reliable, data arrives guaranteed and in order (Redis)
    - UDP = fire and forget (faster but unreliable)
- **connection** is a handshake that client and server do to set up the connection before they start flowing data to each other
    - OS does this, we just use abstractions: *connect()* and *accept()*
- Sent as byte stream not as messages so we need protocol in place to handle this
- TCP produces continuous stream of bytes with no internal boundaries (application protocol has to interpret)
    - it is not messages, it is a byte stream (we need to figure out some way to decide where the messages start and end)
- Data Serialization:
    - **serialization** is object to bytes
    - **deserialization** is bytes to object
- Concurrent Programming
    - We will do event based concurrency (basically we will only use a single thread which does a ton of stuff with a massive to-do list)
- Socket and file descriptor is just a number the OS gives you to refer to a connection
    - **Server side**
        - listening socket
- 4-tuple is just 4 pieces of info in a specific order like (src_ip, src_port, dst_ip, dst_port) 
    - **src_ip** = our computer's IP address
    - **src_port** = local port number our app is using
    - **dst_ip** = server's IP address we are connecting to
    - **dst_port** = server's port number we are connecting to
- Client is the one that recieves and server sends