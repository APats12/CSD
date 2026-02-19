# Multi-Threaded HTTP Server

A concurrent HTTP server written in C++ that handles GET and PUT requests using POSIX threads (pthreads) and dynamic queues for efficient connection handling.

## Overview

This project implements a multi-threaded HTTP server with thread-safe request handling, proper request/response parsing, and graceful shutdown via signal handling. It was built as a systems programming exercise focusing on concurrency, socket programming, and synchronization.

## Features

- **Concurrent request handling** — Worker threads process incoming connections using a dynamic task queue
- **HTTP methods** — Supports GET and PUT with full request parsing and response generation
- **Thread safety** — Mutex locks protect shared data structures and ensure correct concurrent access
- **Graceful shutdown** — Signal handling (e.g., SIGINT) allows clean teardown of threads and resources
- **Logging** — Request and error logging for debugging and monitoring
- **Error handling** — Robust handling of malformed requests and I/O errors

## Tech Stack

- **Language:** C++
- **Concurrency:** pthreads
- **Concepts:** Socket programming, dynamic queues, mutexes, signal handling, HTTP parsing

## Building & Running

### Prerequisites

- A C++ compiler with C++11 or later (e.g., `g++`, `clang++`)
- POSIX environment (Linux, macOS, or WSL)

### Build

```bash
# From the Multi-threadedHTTPServer directory
make
# or
g++ -std=c++11 -pthread -o server *.cpp
```

### Run

```bash
./server [port]
```

Example (default port 8080 or specify e.g. 4000):

```bash
./server 4000
```

Then send requests, e.g.:

```bash
curl -X GET http://localhost:4000/path
curl -X PUT -d "body" http://localhost:4000/path
```

## Project Structure

```
Multi-threadedHTTPServer/
├── README.md
├── Makefile
├── server.cpp          # Main entry, server loop, thread pool
├── request.cpp / .h    # HTTP request parsing
├── response.cpp / .h   # HTTP response generation
└── ...                 # Other source files as in your repo
```

*(Adjust the structure above to match your actual files after you open the repo.)*

## Design Highlights

- **Dynamic queue:** Incoming connections are enqueued; worker threads dequeue and handle them, allowing a fixed number of threads to serve many connections.
- **Mutex usage:** The shared queue and any shared state are protected so that enqueue/dequeue and request handling are thread-safe.
- **Shutdown:** On receiving a shutdown signal, the server stops accepting new connections, drains the queue, joins worker threads, and closes sockets cleanly.

**Repo:** [CSD / Multi-threadedHTTPServer](https://github.com/APats12/CSD/tree/main/Multi-threadedHTTPServer)
