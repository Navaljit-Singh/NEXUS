# NEXUS — C++20 Algorithmic Trading Engine

`[C++20]` `[CMake]` `[Ubuntu 24.04]` `[License: MIT]`

---

## Overview

NEXUS is a high-performance, low-latency algorithmic trading engine written in C++20, purpose-built for Indian equity and derivatives markets (NSE/BSE via Upstox). It features a lock-free event bus, an SPSC-queue-backed logger, a price-level order book, and a modular strategy framework — all designed to minimise GC pauses and syscall overhead. A companion Python research layer (polars + pandas + Jupyter) enables rapid strategy prototyping and backtesting against the same Parquet data files the C++ engine writes. The system targets sub-100 µs end-to-end latency from market feed ingestion through order dispatch.

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        NEXUS Engine                             │
│                                                                 │
│  ┌─────────────┐    MarketEvent    ┌──────────────────────────┐ │
│  │ UpstoxFeed  │ ──────────────▶  │     Event Bus (SPSC)     │ │
│  │  (WS/Proto) │                  └───────────┬──────────────┘ │
│  └─────────────┘                              │                 │
│                              ┌────────────────▼──────────────┐ │
│                              │         OrderBook              │ │
│                              │  (5-level bid/ask ladder)     │ │
│                              └────────────────┬──────────────┘ │
│                                               │                 │
│                              ┌────────────────▼──────────────┐ │
│                              │      Strategy Engine           │ │
│                              │   (EMA, VWAP, custom …)       │ │
│                              └────────────────┬──────────────┘ │
│                                               │                 │
│  ┌─────────────┐             ┌────────────────▼──────────────┐ │
│  │  Logger     │◀────────── │      Order Manager             │ │
│  │ (SPSC queue)│             │   (risk checks → Upstox API)  │ │
│  └─────────────┘             └───────────────────────────────┘ │
│                                                                 │
│  Parquet Writer ──▶ d:\NEXUS\data\  ◀── Python Research Layer  │
└─────────────────────────────────────────────────────────────────┘
```

---

## Repository Layout

```
NEXUS/
├── CMakeLists.txt              # Top-level CMake project
├── CMakePresets.json           # debug / release / asan presets
├── README.md
├── configs/
│   └── nexus_config.json       # Runtime configuration
├── include/
│   └── nexus/
│       ├── core/
│       │   ├── types.hpp       # Fundamental type aliases
│       │   ├── time.hpp        # TimeUtil, epoch_ns()
│       │   ├── logger.hpp      # Async SPSC logger
│       │   ├── event.hpp       # TickData, BarData, BookUpdate, MarketEvent
│       │   └── spsc_queue.hpp  # Lock-free SPSC ring buffer
│       ├── market_data/
│       │   └── upstox_feed.hpp # WebSocket feed client
│       ├── order_book/
│       │   ├── level.hpp       # PriceLevel
│       │   └── order_book.hpp  # 5-level order book
│       ├── strategy/
│       │   ├── strategy_base.hpp
│       │   └── ema_strategy.hpp
│       └── storage/
│           └── parquet_writer.hpp
├── src/
│   ├── core/
│   │   └── logger.cpp
│   ├── market_data/
│   │   └── upstox_feed.cpp
│   ├── order_book/
│   │   └── order_book.cpp
│   ├── strategy/
│   │   └── ema_strategy.cpp
│   ├── storage/
│   │   └── parquet_writer.cpp
│   └── main.cpp
├── tests/
│   ├── test_order_book.cpp
│   ├── test_logger.cpp
│   └── test_metrics.cpp
├── python/
│   ├── requirements.txt
│   └── nexus_research/
│       ├── __init__.py
│       ├── data_loader.py      # ParquetDataLoader (polars)
│       ├── backtest.py         # SimpleBacktester + EMA crossover
│       └── metrics.py          # Sharpe, Sortino, Calmar, max drawdown
├── scripts/
│   └── setup_wsl.sh            # WSL bootstrap script
└── data/                       # Parquet tick/bar files (gitignored)
```

---

## Prerequisites

Ubuntu 24.04 (WSL or native). Install all dependencies in one command:

```bash
sudo apt-get install -y \
    build-essential cmake ninja-build git \
    clang clang-format clang-tidy \
    libssl-dev libboost-all-dev \
    libprotobuf-dev protobuf-compiler \
    libpqxx-dev postgresql-client \
    python3-pip python3-venv
```

Or run the provided bootstrap script (see **Setup**).

---

## Setup

```bash
bash /mnt/d/NEXUS/scripts/setup_wsl.sh
```

This installs system packages and creates a Python virtual environment at `python/.venv`.

---

## Build Instructions

```bash
# Configure (release build with -O3 and LTO)
cmake --preset release

# Compile
cmake --build --preset release

# Optional: address-sanitiser build
cmake --preset asan && cmake --build --preset asan
```

Build artefacts land in `build/release/` and `build/asan/` respectively.

---

## Running

```bash
./build/release/nexus_engine --config configs/nexus_config.json
```

Pass `--help` for a full list of CLI options.

---

## Testing

```bash
./build/release/nexus_tests
# or via CTest
ctest --preset release --output-on-failure
```

---

## Configuration

Edit `configs/nexus_config.json` to set:

| Key | Description |
|-----|-------------|
| `auth_token` | Upstox OAuth2 access token |
| `instruments` | List of instrument keys to subscribe |
| `log_level` | `DEBUG` / `INFO` / `WARN` / `ERROR` |
| `data_dir` | Directory for Parquet output |
| `risk.max_order_value` | Per-order value cap (INR) |
| `risk.daily_loss_limit` | Maximum daily loss before engine halts (INR) |

---

## Python Research

```bash
cd python
source .venv/bin/activate
jupyter lab
```

Key modules:

| Module | Purpose |
|--------|---------|
| `nexus_research.data_loader.ParquetDataLoader` | Load tick/bar Parquet files |
| `nexus_research.backtest.SimpleBacktester` | EMA crossover event-driven backtest |
| `nexus_research.metrics` | Sharpe, Sortino, Calmar, max drawdown |

---

## 20-Week Roadmap

| Phase | Weeks | Focus | Key Deliverables |
|-------|-------|-------|-----------------|
| 1 | 1–2 | Foundation | CMake presets, SPSC queue, logger, type aliases, CI skeleton |
| 2 | 3–4 | Market Data | Upstox WebSocket feed, Protobuf decode, Parquet writer |
| 3 | 5–6 | Order Book | 5-level price-ladder, BBO tracking, unit tests |
| 4 | 7–8 | Strategy Framework | `StrategyBase` interface, EMA crossover strategy, signal bus |
| 5 | 9–10 | Order Management | Paper-trade order manager, risk checks, fill simulation |
| 6 | 11–13 | Python Research | `ParquetDataLoader`, `SimpleBacktester`, metrics library, Jupyter notebooks |
| 7 | 14–17 | Live Integration | Upstox order placement API, live P&L tracking, alerting |
| 8 | 18–20 | Hardening | Latency benchmarks, ASAN/TSAN clean, config hot-reload, production runbook |

---

## Contributing

Pull requests are welcome. Please run `clang-format` and `clang-tidy` before submitting, and ensure all tests pass with both the `release` and `asan` presets.

---

## License

MIT © 2024 NEXUS Contributors
