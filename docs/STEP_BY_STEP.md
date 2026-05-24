# Step-by-Step Project Walkthrough

This project is intentionally built in layers so you can explain it clearly in a resume discussion or interview.

## 1. Start With The Trading Model

The core domain objects are in `include/order.hpp`.

We model:

- `Order`: a buy or sell limit order
- `Trade`: a match between an aggressor order and a resting order
- `ExecutionReport`: the status message returned after order activity
- `BookSnapshot`: top levels of the current market

Prices are stored as integer ticks instead of floating point values. This avoids rounding errors, which is standard practice in trading systems.

## 2. Build A Single-Symbol Order Book

The matching rules live in `include/order_book.hpp` and `src/order_book.cpp`.

The book keeps:

- Bids sorted from highest price to lowest price
- Asks sorted from lowest price to highest price
- FIFO queues at each price level
- An index from order id to price level for fast cancel lookup

This implements price-time priority:

1. Better price wins first.
2. If price is the same, older resting order wins first.

## 3. Submit A Limit Order

When a new order arrives:

1. Validate symbol, price, quantity, and duplicate order id.
2. Acknowledge the order with an `ACCEPTED` execution report.
3. Check whether it crosses the opposite side.
4. Match against the best opposite price while quantity remains.
5. Generate trade records and fill reports.
6. Rest any remaining quantity in the book.

Example:

```text
Resting ask: SELL 50 @ 101
Incoming:    BUY  50 @ 105
Trade:       50 @ 101
```

The trade happens at the resting price, which is how many exchange books behave.

## 4. Add Cancels

Cancel flow uses the order id index.

1. Find the order's side and price level.
2. Remove it from the FIFO queue.
3. Remove the empty price level if needed.
4. Return a `CANCELED` execution report.

If the order is unknown, the engine returns a `REJECTED` report.

## 5. Add Multi-Symbol Routing

`MatchingEngine` manages one `OrderBook` per symbol.

Each book has its own mutex. That means `AAPL` and `MSFT` orders can be submitted concurrently without fighting over one global lock for the matching logic.

This is a simplified version of how real matching systems often partition work by instrument.

## 6. Add FIX-Like Input

`FixParser` accepts simple pipe-delimited messages:

```text
35=D|11=1001|55=AAPL|54=1|38=100|44=18150
35=F|41=1001
```

This is not a full QuickFIX session layer. It is deliberately small so the project demonstrates gateway knowledge without forcing interviewers to install QuickFIX.

Mapping:

- `35=D`: NewOrderSingle
- `35=F`: OrderCancelRequest
- `11`: client order id
- `41`: original client order id for cancel
- `55`: symbol
- `54`: side
- `38`: quantity
- `44`: price

## 7. Add Tests

The tests in `tests/order_book_tests.cpp` cover:

- Non-crossing orders resting on the book
- Full fills
- Partial fills
- FIFO priority at the same price
- Cancel behavior
- Multi-symbol routing

Run:

```powershell
mingw32-make test
```

## 8. Add A Demo

The CLI app reads `samples/orders.fix`, submits each message, and prints:

- Execution reports
- Trades
- Final book snapshots

Run:

```powershell
mingw32-make
build\matching_engine.exe samples\orders.fix
```

## Interview Explanation

Use this short version:

> I built a C++17 matching engine that supports limit orders, price-time priority, partial fills, cancels, execution reports, and top-of-book snapshots. The matching core is deterministic per symbol, while the engine routes orders to per-symbol books protected by independent mutexes. I also added a FIX-like parser so the project connects directly to exchange gateway workflows I use at work.

## Good Next Feature To Add

The strongest next feature is order amend/replace:

```text
35=G|11=new_id|41=old_id|38=new_qty|44=new_px
```

That gives you a good interview discussion about preserving priority when quantity is reduced versus losing priority when price changes.
