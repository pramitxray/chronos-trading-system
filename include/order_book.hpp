#pragma once

#include "order.hpp"

#include <deque>
#include <functional>
#include <map>
#include <optional>
#include <unordered_map>

namespace ome {

class OrderBook {
public:
    explicit OrderBook(std::string symbol);

    const std::string& symbol() const;

    SubmitResult submit(Order order);
    ExecutionReport cancel(OrderId order_id);
    BookSnapshot snapshot(std::size_t depth = 5) const;

private:
    using OrderQueue = std::deque<Order>;
    using BidBook = std::map<Price, OrderQueue, std::greater<Price>>;
    using AskBook = std::map<Price, OrderQueue>;

    struct OrderLocation {
        Side side;
        Price price;
    };

    bool crosses(const Order& incoming) const;
    Price best_opposite_price(Side side) const;
    OrderQueue& best_opposite_queue(Side side);
    void erase_best_if_empty(Side side);
    void add_resting_order(Order order);
    std::optional<Order> remove_order(OrderId order_id);
    ExecutionReport make_report(const Order& order, OrderStatus status, Quantity last_quantity, Price last_price, const std::string& text) const;

    std::string symbol_;
    BidBook bids_;
    AskBook asks_;
    std::unordered_map<OrderId, OrderLocation> order_index_;
};

} // namespace ome
