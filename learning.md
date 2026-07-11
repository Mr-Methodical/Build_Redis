# What is Redis?
**in-memory key-value store** = super-fast dictionary that lives on a server and multiple programs can talk to over a network. Used for:
- **Caching** = saving work you already did (ex. query db once instead of 100 times)
- **Session Storage** = server doesn't remember who you are from one click to the next, so a session ID can be stored with your details to keep you logged in as you browse.
- **Leaderboards** = can return rank in near constant time
- **Message Queues** = drops task into redis queue and another worker processes that in the background
- **Rate Limiting** = prevent bots by only allowing so many requests
- It stands for **Re**mote **Di**ctionary **S**erver
    - It is a key value store
    - In memory just means that it stores everything in RAM 
    - typical databases like postgresql store on disk but we store on RAM instead
    - the number one job of it is to cache everything
    - Can track who is logged in currently, live leaderboards, and message queues
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
## Idea
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
## Concepts
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
- ** Networking programming ** is just turning an unreliable stream of bytes into a logical conversation
- Fresh notes (basically me going back over and rereading):
    - write() / send() copies data into the OS **send buffer**
    - read() / recv() checks if anything has arrived yet - OS checks the **Receive buffer**
        - if anything in the OS will copy them over into application memory so we can parse
    - read() and write() are generic so they work for sockets and files 
        > In Unix/linux (core philosophy), "Everything is a file"
    - recv() and send() are socket specific (same as read and write) but they take one more argument at the end:
        - for network specific tricks
        - ex. you can peak at data instead of grabbing it (MSG_PEEK)
    - TCP Byte Stream and Protocols:
        - TCP = Transmission Control Protocol
            - It is reliable as it will send small packets and will ask for the packet again if it gets lost
        - TCP produces a continuous stream of bytes with no internal boundaries
        - We will have an **application protocol** that makes sense of this byte stream
        - UDP = User Datagram Protocol
            - Very efficient as it skips the formal connection process
            - Does not check if packets arrive so can be slightly unreliable
            - The **Datagrams** could arrive out of order
        - **Event loop** is a single thread that manages thousands of connections by rapidly cycling through them (it never stops and waits)
            - We will have to build buffers for each of our waiting rooms
    - Data Serialization
        - objects could be things like strings, structs, lists
        - **Serialization** is objects to bytes
        - **Deserialization** is bytes to objects
        - We won't use JSON or Protobuf so we can be more efficient and not send as many bytes over the network and also have less CPU cycles to decode them
    - Concurrent Programming
        - > **Efficient software is required to be able to make full use of hardware**
        - We are going to do **event-based concurrency** 
            - insanely fast single thread (instead of 10000 threads so less RAM overhead)
            - Some famous examples of software that does Event loops right now
                - NGINX = massive, high speed web server
                - Node.js = runtime that allows you to write javascript on the backend
                - Golang = programming language built to handle massive network concurrency
                - All solve C10k connection problem in same way we are going to
            - non-blocking means that it doesn't freeze when it asks to read(), it is just able to and if nothing there it just moves on instead of waiting for something to come (which would be blocking)
            - we are going to make our sockets non-blocking
            - One interesting problem is that the OS used to have loop through all 10,000 ports but now we solve this with epoll() so basically instead this time when a connection comes we will register it and put it in a dashboard/queue
            - a protocol specification is just the rule book for how they communicate
            - multiplexing = many into one so instead of having 10k ports and each get a new thread we don't do that and instead have our event loop where one does a ton of things
    - Layers of Protocol
        - **Network Protocols** = agreed upon set of rules for communication
        - There are different layers
            - Higher layer adds new functionality
            - A lower layer can contain a higher layer
                - **Encapsulation** (basically we trust lower layer to do its job)
                    - provides layering ("wrapping data" in pre-existing layers)
        - layers
            - IP only handles smaller packets called IP Packets
            - assembling packets into application data is provided by higher layer like TCP 
            - **multiplexing** think port number
                - All this data coming into the computer it needs to know which app to give this data to
                - UDP/TCP will add a 16 bit port number for this distinguish
                    - For this it will use a 4-tuple thing which shows ip of source and port of source and also this for destination
            - TCP for reliability and ordered bytes on IP layer
- Request-response protocols:
    - Each request message paired with a response
    - if we asked what is x, then we expect to get a response like x is 10
    - most want this except DNS which just wants to be fast
        - DNS = Domain Name system it is what translates google.com to a server number
- API = Application Programming Interface
    - An abstraction to use the code (menu of functions)
- Socket Primitives:
    - socket = handle to refer to a connection or something else
    - API for networking is just called the socket API
    - On linux a handle is called a **file descriptor** (fd)
        - Nothing to do with files nor describe anything
    - *socket()* will return a handle used for creating connections
        - must be closed at the end to give the resources back to the OS
    - Listening is saying that you are ready to connect with another port through TCP
    - Then the listening port can accept when something comes in
    - 3 API calls to create a listening socket
        1. *socket()* to obtain socket handle (we get a fd) (ID card for network communication channelt that the OS is handling)
        2. *bind()* tells the OS exactly which port you want and which connection you want it to
