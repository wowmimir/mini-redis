# 🚀 Redis-Inspired In-Memory Database (C++)

A high-performance, event-driven in-memory database server built from scratch in C++, inspired by Redis.

This project explores core systems concepts including non-blocking I/O, event loops, protocol parsing, and persistence.

---

## 🧠 Key Features

### ⚡ Event-Driven Architecture

* Built using Linux `epoll`
* Fully non-blocking I/O
* Single-threaded, high-concurrency design

### 🌐 Networking

* TCP server with multiple concurrent clients
* Handles partial reads/writes correctly
* Request pipelining supported

### 📦 RESP Protocol

* Streaming parser (handles fragmented packets)
* Supports multiple commands per buffer
* RESP-compliant responses

### 🧩 Core Commands

* `SET key value`
* `GET key`
* `DEL key`

### 💾 Persistence (AOF)

* Append-only file for durability
* Database restored on startup
* Atomic AOF rewrite (log compaction)

### 🔁 Write Buffering

* Non-blocking writes using `EPOLLOUT`
* Per-client output buffers
* Handles slow clients safely

---

## 🏗️ Architecture

```
Client
  ↓
TCP Socket
  ↓
Epoll Event Loop
  ↓
Connection Buffers
  ↓
RESP Parser
  ↓
Command Execution
  ↓
In-Memory Store
  ↓
AOF Persistence
  ↓
Write Buffer → Client
```

---

## 📁 Project Structure

```
src/
 ├── server/        # Event loop and networking
 ├── connection/    # Per-client state
 ├── protocol/      # RESP parser
 ├── command/       # Command execution logic
 ├── store/         # In-memory database
 ├── persistence/   # AOF handling
```

---

## ⚙️ Build & Run

### Build

```bash
g++ -std=c++17 -O2 -Wall -o server src/**/*.cpp
```

### Run

```bash
./server
```

Server listens on port `8000`.

---

## 🧪 Example Usage

Using netcat or a custom client:

```
SET x 10
GET x
```

RESP request:

```
*3
$3
SET
$1
x
$2
10
```

---

## 🧠 Design Highlights

### Non-Blocking I/O

All sockets operate in non-blocking mode. The server never blocks on read/write, enabling high concurrency with a single thread.

### Write Buffering

Responses are queued in per-client buffers. `EPOLLOUT` ensures writes happen only when sockets are ready.

### AOF Rewrite

* Compacts command history into minimal state
* Uses a rewrite buffer to avoid losing writes during rewrite
* Atomic file replacement ensures durability

---

## ⚠️ Limitations

* Single-threaded (no thread pool)
* No replication
* No advanced data types (lists, sets, etc.)
* AOF rewrite is blocking (no fork-based rewrite yet)

---

## 🚀 Future Improvements

* Fork-based AOF rewrite (like Redis)
* Backpressure & flow control
* TTL (key expiration)
* Replication (master-slave)
* Advanced data structures

---

## 🎯 Learning Outcomes

This project demonstrates:

* Low-level systems programming (C++)
* Event-driven server design
* Streaming protocol parsing
* Non-blocking I/O and epoll
* Persistence and crash safety trade-offs

---

## 📌 Inspiration

Inspired by Redis and its event-driven architecture.

---

## 📜 License

MIT License
