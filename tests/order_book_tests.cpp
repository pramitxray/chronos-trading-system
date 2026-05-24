#include "matching_engine.hpp"
#include "order_book.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << "\n";
    }
}

ome::Order make_order(ome::OrderId id, ome::Side side, ome::Quantity quantity, ome::Price price) {
    return ome::Order{id, std::to_string(id), "AAPL", side, quantity, quantity, price, id};
}

void test_non_crossing_orders_rest_on_book() {
    ome::OrderBook book("AAPL");
    book.submit(make_order(1, ome::Side::Buy, 100, 100));
    book.submit(make_order(2, ome::Side::Sell, 40, 105));

    const auto snapshot = book.snapshot();
    expect(snapshot.bids.size() == 1, "one bid level expected");
    expect(snapshot.asks.size() == 1, "one ask level expected");
    expect(snapshot.bids[0].price == 100 && snapshot.bids[0].quantity == 100, "bid level should aggregate quantity");
    expect(snapshot.asks[0].price == 105 && snapshot.asks[0].quantity == 40, "ask level should aggregate quantity");
}

void test_crossing_order_fills_at_resting_price() {
    ome::OrderBook book("AAPL");
    book.submit(make_order(1, ome::Side::Sell, 50, 101));
    const auto result = book.submit(make_order(2, ome::Side::Buy, 50, 105));

    expect(result.trades.size() == 1, "one trade expected");
    expect(result.trades[0].price == 101, "trade should execute at resting order price");
    expect(result.trades[0].quantity == 50, "trade quantity should match both orders");
    expect(book.snapshot().asks.empty(), "ask should be removed after full fill");
}

void test_partial_fill_leaves_remainder() {
    ome::OrderBook book("AAPL");
    book.submit(make_order(1, ome::Side::Sell, 100, 101));
    const auto result = book.submit(make_order(2, ome::Side::Buy, 30, 101));

    expect(result.trades.size() == 1, "partial fill should trade once");
    const auto snapshot = book.snapshot();
    expect(snapshot.asks.size() == 1, "resting ask remainder should stay");
    expect(snapshot.asks[0].quantity == 70, "remaining ask quantity should be 70");
}

void test_fifo_priority_at_same_price() {
    ome::OrderBook book("AAPL");
    book.submit(make_order(1, ome::Side::Sell, 10, 101));
    book.submit(make_order(2, ome::Side::Sell, 10, 101));
    const auto result = book.submit(make_order(3, ome::Side::Buy, 15, 101));

    expect(result.trades.size() == 2, "incoming buy should match two resting asks");
    expect(result.trades[0].resting_order_id == 1, "first resting order should fill first");
    expect(result.trades[1].resting_order_id == 2, "second resting order should fill second");
    expect(book.snapshot().asks[0].quantity == 5, "second order should have 5 remaining");
}

void test_cancel_removes_order() {
    ome::OrderBook book("AAPL");
    book.submit(make_order(1, ome::Side::Buy, 100, 100));
    const auto report = book.cancel(1);

    expect(report.status == ome::OrderStatus::Canceled, "cancel should return canceled status");
    expect(book.snapshot().bids.empty(), "canceled order should leave no bid level");
}

void test_matching_engine_routes_by_symbol() {
    ome::MatchingEngine engine;
    engine.submit(ome::FixOrderMessage{ome::FixMessageType::NewOrderSingle, "a", "", "AAPL", ome::Side::Buy, 100, 100});
    engine.submit(ome::FixOrderMessage{ome::FixMessageType::NewOrderSingle, "m", "", "MSFT", ome::Side::Sell, 100, 200});

    expect(engine.snapshot("AAPL").bids.size() == 1, "AAPL bid should exist");
    expect(engine.snapshot("MSFT").asks.size() == 1, "MSFT ask should exist");
}

} // namespace

int main() {
    test_non_crossing_orders_rest_on_book();
    test_crossing_order_fills_at_resting_price();
    test_partial_fill_leaves_remainder();
    test_fifo_priority_at_same_price();
    test_cancel_removes_order();
    test_matching_engine_routes_by_symbol();

    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "all tests passed\n";
    return EXIT_SUCCESS;
}
