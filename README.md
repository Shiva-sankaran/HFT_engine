# ⚡ High-Performance Trade Processing Engine

This project implements a multi-threaded trade processing engine in modern C++17. It is designed to simulate a low-latency trading system that receives trade data over TCP, processes it in real-time, and generates alerts based on VWAP deviations.

## 🚀 Current Performance

- **Total average latency:** ~1.3 µs
- **Wait latency (queue contention):** ~0.7 µs
- **Processing latency:** ~0.5 µs
- **P50 latency:** ~966 ns  
- **P90 latency:** ~1.5 µs  
- **P99 latency:** ~4.3 µs  

> ⚠️ Occasional latency spikes observed due to CPU contention on local loopback: max ~16 µs

---

## 📦 Overview

The system consists of the following components:

| Component        | Description                                                                 |
|------------------|-----------------------------------------------------------------------------|
| `client`         | Connects to a TCP server, receives trade data as JSON, parses and timestamps it |
| `TradeEngine`    | Core engine that manages dispatching, threading, and trade processing       |
| `ThreadSafeQueue`| A blocking queue used for inter-thread communication                        |
| `TradeEvent`     | Struct representing an individual trade                                     |
| `VWAP Processor` | Maintains a sliding window per symbol to compute real-time VWAP and alerts  |
| `Latency Monitor`| Tracks wait, processing, and total latencies; logs them to CSV              |

---

## 🏗️ Architecture

          +--------------------+
          |   TCP Trade Feed   |
          +--------+-----------+
                   |
            [client.cpp] (receives trade JSONs)
                   |
          +--------v-----------+
          | mainDataQueue (TSQ)|
          +--------+-----------+
                   |
          +--------v---------------------+
          | Dispatcher Thread            |
          |  - Symbol-to-worker mapping  |
          +------------------------------+
                     |
          +----------v----------+   +----------v----------+   ...
          | Worker Queue (TSQ)  |   | Worker Queue (TSQ)  |
          +----------+----------+   +----------+----------+
                     |                      |
        +------------v------------+ +--------v-----------+   ...
        | process_trade()         | | process_trade()    |
        |  - VWAP + Alert Logic   | |  + Latency Tracking |
        +-------------------------+ +---------------------+

---

## 🧪 Metrics Tracked

Each trade is measured across:

- **Wait Latency**: Time from when the trade is received to when it is picked by a worker
- **Processing Latency**: Time spent inside `process_trade`
- **Total Latency**: End-to-end latency = wait + processing

A CSV log `latency_log.csv` is generated with per-trade latency.

The `print_summary()` function displays:
- Total trades and alerts
- Average and percentile latencies
- Per-symbol trade stats
- Symbol-to-worker mapping

---

## 🚀 Running Instructions

### 1. 🔧 Compile

You can compile using `g++` or `clang++`:

```bash
g++ -std=c++17 -O2 -pthread -o trade_app main.cpp client.cpp trade_engine.cpp -lfmt
```

Make sure:

* You have installed libfmt (e.g., via ```sudo apt install libfmt-dev```)

* Your compiler supports C++17


### 2. 🧠 Start Trade Feed Server
Run the Python trade sender:

```bash
python3 trade_sender.py
```

This connects to port ```5000``` and sends JSON-formatted trade events.


###  3. 📡 Run the Engine
Start the engine (after compiling):


```bash
./trade_app
```

You will see logs showing:

* Symbol mapping

* VWAP computation

* Latency stats

* Alert messages if thresholds are breached


### ⚠️ Known Design Decisions

* Dispatcher uses symbol → i % n_workers mapping for load balancing

* Trade queues are std::deque-based thread-safe blocking queues (1 per worker)

* One dispatcher thread pushes to worker queues

* Trades are timestamped right before enqueuing for accurate latency tracking


###  📊 Sample Output

```
Total trades: 5000
Total alerts: 0
Total Latency: 205µs
Wait Latency: 200µs
Processing Latency: 4µs
P50 latency: 199µs
P90 latency: 230µs
P99 latency: 310µs
```

### 📌 TODO: Future Improvements

| Feature                        | Description                                                                 |
|-------------------------------|-----------------------------------------------------------------------------|
| 🔄 Replace dispatcher          | Avoid central dispatcher by routing trades directly to queues (e.g., symbol-hash) |
| 🧵 Lock-free queues            | Implement SPSC or MPSC ring buffers to reduce wait latency                 |
| ⚡ Busy-wait loops             | Replace `pop_blocking()` with poll-based low-latency hot-loop              |
| 📉 Real-time metrics dashboard| Add live latency tracking with Prometheus + Grafana or simple `curses` UI |
| 📈 Latency heatmap visuals    | Plot latency per symbol/thread for debugging                               |
| 🧪 Unit/Integration tests     | Add GoogleTest-based test suite                                            |
| 🧬 Load testing               | Generate high volume trade bursts to benchmark thread scalability          |
| 🧠 Symbol affinity optimization | Pin threads to cores based on symbol assignment to improve cache locality |

---

### 🧠 Learnings So Far

- Thread partitioning reduces contention when trade volume per symbol is high  
- Dispatcher introduces bottleneck when symbols are too dynamic or bursty  
- Wait latency dominates total latency if threads are underutilized or queueing is slow  
- VWAP calculation with deques is efficient for sliding-window stats  


---

### 🤝 Contributors

- **Shivasankaran** – System design, implementation, performance benchmarking

---

### 📜 License

MIT License – Use and modify freely.


---
---
---
---
---

## 🛠️ Future Work

Here are important areas for improvement and extension to move the system toward production-grade HFT performance:

### 🧵 Threading & Queues

- **🔄 Eliminate Dispatcher Thread**
  - Replace dispatcher with direct hashing (`symbol → queue`) to avoid bottlenecks
  - Use a consistent hash or radix-based mapping scheme

- **⚡ Lock-Free Queues**
  - Replace `ThreadSafeQueue` with single-producer-single-consumer (SPSC) or multi-producer-single-consumer (MPSC) ring buffers
  - Reduces contention and wake-up delays in hot paths

- **🧬 CPU Affinity and Core Pinning**
  - Pin each worker thread to a fixed core to improve cache locality and reduce context switching

- **📈 Dynamic Load Balancing**
  - Add logic to rebalance symbols across threads dynamically if volume per symbol is skewed

---

### 🧪 Benchmarking & Instrumentation

- **📉 Granular Profiling**
  - Break down wait vs. processing latency per thread
  - Log queue depth and timestamp deltas for performance analysis

- **📊 Visualization Tools**
  - Add scripts (Python/Matplotlib) to analyze `latency_log.csv`
  - Plot P50/P90/P99 over time or per-symbol

- **📉 Realistic Load Testing**
  - Create synthetic bursty trade feeds with clustered spikes to test stress limits

---

### 📡 Network & Ingestion

- **🌐 Non-blocking Socket Ingestion**
  - Switch client ingestion to `epoll`/`select` for better throughput
  - Support multiple clients and simulate co-located vs. remote latency

- **🧩 Kafka or Shared Memory Ingestion (Advanced)**
  - Replace socket input with real pub-sub (e.g., Kafka, ZeroMQ, shared memory segment)

---

### ⚙️ Alerts and Strategy Logic

- **📈 Real Strategy Plugins**
  - Make alert logic pluggable: VWAP, TWAP, Bollinger, etc.
  - Allow injecting trading logic via strategy interface

- **🧪 Backtesting Mode**
  - Support replaying historical trades at configurable speeds with mock clocks

---

### 🧹 Code Quality & DevX

- **✅ Add Unit Tests**
  - Write test cases for `TradeEvent`, VWAP logic, queue correctness, etc.

- **🛠️ CI/CD Integration**
  - Add GitHub Actions or Makefile to automate builds and linting

- **📦 CMake Support**
  - Replace raw g++ compile line with proper `CMakeLists.txt` for better scalability

---

### 📦 Advanced Features

- **📊 Prometheus Exporter**
  - Export real-time stats (latency, queue depth, alerts/sec) to Prometheus/Grafana

- **🧪 Fault Injection Framework**
  - Simulate message drops, clock skews, high-latency trades to test robustness

- **🧠 ML-Based Anomaly Detection**
  - Plug in a lightweight model to classify trades as anomalous or not based on price/volume/latency

---

