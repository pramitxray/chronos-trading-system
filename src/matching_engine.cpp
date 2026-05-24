#include "matching_engine.hpp"

#include <utility>

namespace ome {

SubmitResult MatchingEngine::submit(const FixOrderMessage& message) {
    if (message.type == FixMessageType::CancelRequest) {
        SubmitResult result;
        result.reports.push_back(cancel_by_client_order_id(message.original_client_order_id));
        return result;
    }

    Order order;
    {
        std::lock_guard<std::mutex> lock(engine_mutex_);
        if (client_to_order_id_.count(message.client_order_id) != 0) {
            SubmitResult result;
            ExecutionReport report;
            report.client_order_id = message.client_order_id;
            report.symbol = message.symbol;
            report.side = message.side;
            report.status = OrderStatus::Rejected;
            report.text = "duplicate client order id";
            result.reports.push_back(report);
            return result;
        }
        order.id = next_order_id_++;
        order.sequence = next_sequence_++;
        order.client_order_id = message.client_order_id;
        order.symbol = message.symbol;
        order.side = message.side;
        order.quantity = message.quantity;
        order.leaves_quantity = message.quantity;
        order.price = message.price;
        client_to_order_id_[order.client_order_id] = order.id;
        order_to_client_id_[order.id] = order.client_order_id;
        order_to_symbol_[order.id] = order.symbol;
    }

    auto& state = book_for(order.symbol);
    SubmitResult result;
    {
        std::lock_guard<std::mutex> book_lock(state.mutex);
        result = state.book.submit(order);
    }

    for (const auto& report : result.reports) {
        if (report.status == OrderStatus::Filled || report.status == OrderStatus::Canceled || report.status == OrderStatus::Rejected) {
            std::lock_guard<std::mutex> lock(engine_mutex_);
            const auto client = order_to_client_id_.find(report.order_id);
            if (client != order_to_client_id_.end()) {
                client_to_order_id_.erase(client->second);
                order_to_client_id_.erase(client);
                order_to_symbol_.erase(report.order_id);
            }
        }
    }

    return result;
}

ExecutionReport MatchingEngine::cancel_by_client_order_id(const std::string& client_order_id) {
    OrderId order_id = 0;
    std::string symbol;
    {
        std::lock_guard<std::mutex> lock(engine_mutex_);
        const auto order_it = client_to_order_id_.find(client_order_id);
        if (order_it == client_to_order_id_.end()) {
            ExecutionReport report;
            report.client_order_id = client_order_id;
            report.status = OrderStatus::Rejected;
            report.text = "unknown client order id";
            return report;
        }
        order_id = order_it->second;
        const auto symbol_it = order_to_symbol_.find(order_id);
        if (symbol_it != order_to_symbol_.end()) {
            symbol = symbol_it->second;
        }
    }

    if (!symbol.empty()) {
        auto& state = book_for(symbol);
        std::lock_guard<std::mutex> book_lock(state.mutex);
        auto report = state.book.cancel(order_id);
        if (report.status == OrderStatus::Canceled) {
            std::lock_guard<std::mutex> lock(engine_mutex_);
            client_to_order_id_.erase(client_order_id);
            order_to_client_id_.erase(order_id);
            order_to_symbol_.erase(order_id);
            return report;
        }
    }

    std::lock_guard<std::mutex> lock(engine_mutex_);
    client_to_order_id_.erase(client_order_id);
    order_to_client_id_.erase(order_id);
    order_to_symbol_.erase(order_id);
    ExecutionReport report;
    report.order_id = order_id;
    report.client_order_id = client_order_id;
    report.status = OrderStatus::Rejected;
    report.text = "order not found in any book";
    return report;
}

BookSnapshot MatchingEngine::snapshot(const std::string& symbol, std::size_t depth) const {
    std::lock_guard<std::mutex> engine_lock(engine_mutex_);
    const auto book = books_.find(symbol);
    if (book == books_.end()) {
        BookSnapshot empty;
        empty.symbol = symbol;
        return empty;
    }

    std::lock_guard<std::mutex> book_lock(book->second->mutex);
    return book->second->book.snapshot(depth);
}

MatchingEngine::BookState& MatchingEngine::book_for(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    auto [it, inserted] = books_.try_emplace(symbol, nullptr);
    if (inserted || it->second == nullptr) {
        it->second = std::make_unique<BookState>(symbol);
    }
    return *it->second;
}

} // namespace ome
