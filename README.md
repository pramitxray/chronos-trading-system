# Order Matching Engine

A C++17 limit order matching engine built as a resume project for exchange gateway and trading-system roles.

Inspired by `zzsun777/cpp_multithreaded_order_matching_engine`, with a smaller modern codebase, dependency-free build, tests, and interview-friendly documentation.

## Features

- Price-time priority central limit order book
- Buy and sell limit orders
- Partial fills and full fills
- Cancel by order id
- Order amend/replace with FIX-like `35=G`
- Per-symbol matching books managed by a concurrent engine
- Thread-safe order submission
- FIX-like input parser for `35=D` new orders, `35=F` cancels, and `35=G` amends
- Execution reports and trade records
- Market-data style top-of-book snapshots
- Unit-style regression tests without external test frameworks
- Demo scenario suitable for resume screenshots and walkthroughs

## Build

```powershell
mingw32-make
```

or directly:

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic -Iinclude src\*.cpp app\main.cpp -o build\matching_engine.exe
```

## Run Demo

```powershell
build\matching_engine.exe samples\orders.fix
```

## Run Tests

```powershell
mingw32-make test
```

## Sample FIX-like Messages

The parser accepts pipe-delimited messages:

```text
35=D|11=1001|55=AAPL|54=1|38=100|44=18150
35=G|11=1001R|41=1001|55=AAPL|54=1|38=80|44=18150
35=F|41=1001
```

Important tags:

- `35=D`: new order single
- `35=F`: cancel request
- `35=G`: amend/replace request
- `11`: client order id
- `41`: original client order id for cancel or amend
- `55`: symbol
- `54`: side, `1` buy and `2` sell
- `38`: quantity
- `44`: limit price in integer ticks

## Architecture

```text
FIX-like input -> FixParser -> MatchingEngine -> OrderBook per symbol
                                      |
                                      v
                         ExecutionReport + Trade events
```

The `OrderBook` owns the matching rules for one symbol. `MatchingEngine` routes messages to the correct book and protects each book with a mutex, so independent symbols can be submitted concurrently.

## Resume Talking Points

- Implemented price-time-priority matching with partial fills, cancels, execution reports, and top-of-book snapshots in modern C++.
- Added amend/replace handling where same-price quantity reductions preserve FIFO priority, while price changes and quantity increases reset priority.
- Designed a concurrent per-symbol engine to model exchange matching partitioning while keeping each order book deterministic.
- Added a FIX-like parser to connect matching logic with real exchange-gateway concepts such as NewOrderSingle, OrderCancelRequest, and OrderCancelReplaceRequest.
- Built regression tests for crossing orders, partial fills, FIFO priority, cancellation, amend behavior, and multi-symbol routing.

## Possible Extensions

- Add market orders and IOC/FOK time-in-force
- Add persistence or replay from an append-only event log
- Add latency benchmark with generated traffic
- Add a real QuickFIX adapter around the existing `MatchingEngine`
