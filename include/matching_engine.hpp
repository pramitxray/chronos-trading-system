#pragma once

#include "fix_parser.hpp"
#include "order_book.hpp"

#include <memory>
#include <mutex>
#include <unordered_map>

namespace ome {

class MatchingEngine {
public:
    SubmitResult submit(const FixOrderMessage& message);
    ExecutionReport cancel_by_client_order_id(const std::string& client_order_id);
    BookSnapshot snapshot(const std::string& symbol, std::size_t depth = 5) const;

private:
    struct BookState {
        mutable std::mutex mutex;
        OrderBook book;

        explicit BookState(std::string symbol) : book(std::move(symbol)) {}
    };

    BookState& book_for(const std::string& symbol);

    mutable std::mutex engine_mutex_;
    std::unordered_map<std::string, std::unique_ptr<BookState>> books_;
    std::unordered_map<std::string, OrderId> client_to_order_id_;
    std::unordered_map<OrderId, std::string> order_to_client_id_;
    std::unordered_map<OrderId, std::string> order_to_symbol_;
    OrderId next_order_id_{1};
    std::uint64_t next_sequence_{1};
};

} // namespace ome
