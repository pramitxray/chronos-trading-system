#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ome {

using OrderId = std::uint64_t;
using Quantity = std::uint64_t;
using Price = std::int64_t;

enum class Side {
    Buy,
    Sell
};

enum class OrderStatus {
    Accepted,
    PartiallyFilled,
    Filled,
    Canceled,
    Rejected
};

struct Order {
    OrderId id{};
    std::string client_order_id;
    std::string symbol;
    Side side{Side::Buy};
    Quantity quantity{};
    Quantity leaves_quantity{};
    Price price{};
    std::uint64_t sequence{};
};

struct Trade {
    std::string symbol;
    OrderId aggressor_order_id{};
    OrderId resting_order_id{};
    Price price{};
    Quantity quantity{};
};

struct ExecutionReport {
    OrderId order_id{};
    std::string client_order_id;
    std::string symbol;
    Side side{Side::Buy};
    OrderStatus status{OrderStatus::Accepted};
    Price limit_price{};
    Price last_price{};
    Quantity order_quantity{};
    Quantity last_quantity{};
    Quantity cumulative_quantity{};
    Quantity leaves_quantity{};
    std::string text;
};

struct SubmitResult {
    std::vector<ExecutionReport> reports;
    std::vector<Trade> trades;
};

struct BookLevel {
    Price price{};
    Quantity quantity{};
};

struct BookSnapshot {
    std::string symbol;
    std::vector<BookLevel> bids;
    std::vector<BookLevel> asks;
};

std::string to_string(Side side);
std::string to_string(OrderStatus status);

} // namespace ome
