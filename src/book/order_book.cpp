#include "ome/book/order_book.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace ome {

OrderBook::OrderBook(std::string symbol) : symbol_(std::move(symbol)) {}

const std::string& OrderBook::symbol() const {
    return symbol_;
}

SubmitResult OrderBook::submit(Order order) {
    SubmitResult result;

    if (order.symbol != symbol_) {
        result.reports.push_back(make_report(order, OrderStatus::Rejected, 0, 0, "symbol does not match book"));
        return result;
    }
    if (order.quantity == 0 || order.leaves_quantity == 0 || order.price <= 0) {
        result.reports.push_back(make_report(order, OrderStatus::Rejected, 0, 0, "quantity and price must be positive"));
        return result;
    }
    if (order_index_.count(order.id) != 0) {
        result.reports.push_back(make_report(order, OrderStatus::Rejected, 0, 0, "duplicate order id"));
        return result;
    }

    result.reports.push_back(make_report(order, OrderStatus::Accepted, 0, 0, "accepted"));
    match(order, result);

    return result;
}

SubmitResult OrderBook::amend(OrderId order_id, std::string new_client_order_id, Quantity new_quantity, Price new_price, std::uint64_t sequence) {
    SubmitResult result;
    Order* current = find_order(order_id);
    if (current == nullptr) {
        ExecutionReport report;
        report.order_id = order_id;
        report.client_order_id = std::move(new_client_order_id);
        report.symbol = symbol_;
        report.status = OrderStatus::Rejected;
        report.text = "order not found";
        result.reports.push_back(report);
        return result;
    }

    if (new_quantity == 0 || new_price <= 0) {
        result.reports.push_back(make_report(*current, OrderStatus::Rejected, 0, 0, "quantity and price must be positive"));
        return result;
    }

    const Quantity cumulative_quantity = current->quantity - current->leaves_quantity;
    if (new_quantity <= cumulative_quantity) {
        result.reports.push_back(make_report(*current, OrderStatus::Rejected, 0, 0, "amended quantity must exceed filled quantity"));
        return result;
    }

    const bool preserves_priority = new_price == current->price && new_quantity <= current->quantity;
    if (preserves_priority) {
        current->client_order_id = std::move(new_client_order_id);
        current->quantity = new_quantity;
        current->leaves_quantity = new_quantity - cumulative_quantity;
        result.reports.push_back(make_report(*current, OrderStatus::Replaced, 0, 0, "replaced; priority preserved"));
        return result;
    }

    auto replacement = remove_order(order_id);
    if (!replacement.has_value()) {
        ExecutionReport report;
        report.order_id = order_id;
        report.client_order_id = std::move(new_client_order_id);
        report.symbol = symbol_;
        report.status = OrderStatus::Rejected;
        report.text = "order not found";
        result.reports.push_back(report);
        return result;
    }

    replacement->client_order_id = std::move(new_client_order_id);
    replacement->quantity = new_quantity;
    replacement->leaves_quantity = new_quantity - cumulative_quantity;
    replacement->price = new_price;
    replacement->sequence = sequence;

    result.reports.push_back(make_report(*replacement, OrderStatus::Replaced, 0, 0, "replaced; priority reset"));
    match(*replacement, result);
    return result;
}

ExecutionReport OrderBook::cancel(OrderId order_id) {
    const auto removed = remove_order(order_id);
    if (!removed.has_value()) {
        ExecutionReport report;
        report.order_id = order_id;
        report.symbol = symbol_;
        report.status = OrderStatus::Rejected;
        report.text = "order not found";
        return report;
    }
    return make_report(*removed, OrderStatus::Canceled, 0, 0, "canceled");
}

BookSnapshot OrderBook::snapshot(std::size_t depth) const {
    BookSnapshot result;
    result.symbol = symbol_;

    for (const auto& [price, orders] : bids_) {
        Quantity total = 0;
        for (const auto& order : orders) {
            total += order.leaves_quantity;
        }
        if (total > 0) {
            result.bids.push_back(BookLevel{price, total});
        }
        if (result.bids.size() >= depth) {
            break;
        }
    }

    for (const auto& [price, orders] : asks_) {
        Quantity total = 0;
        for (const auto& order : orders) {
            total += order.leaves_quantity;
        }
        if (total > 0) {
            result.asks.push_back(BookLevel{price, total});
        }
        if (result.asks.size() >= depth) {
            break;
        }
    }

    return result;
}

bool OrderBook::crosses(const Order& incoming) const {
    if (incoming.side == Side::Buy) {
        return !asks_.empty() && incoming.price >= asks_.begin()->first;
    }
    return !bids_.empty() && incoming.price <= bids_.begin()->first;
}

Price OrderBook::best_opposite_price(Side side) const {
    if (side == Side::Buy) {
        return asks_.begin()->first;
    }
    return bids_.begin()->first;
}

OrderBook::OrderQueue& OrderBook::best_opposite_queue(Side side) {
    const Price price = best_opposite_price(side);
    if (side == Side::Buy) {
        return asks_.at(price);
    }
    return bids_.at(price);
}

void OrderBook::erase_best_if_empty(Side side) {
    if (side == Side::Buy) {
        auto best = asks_.begin();
        if (best != asks_.end() && best->second.empty()) {
            asks_.erase(best);
        }
    } else {
        auto best = bids_.begin();
        if (best != bids_.end() && best->second.empty()) {
            bids_.erase(best);
        }
    }
}

void OrderBook::add_resting_order(Order order) {
    order_index_[order.id] = OrderLocation{order.side, order.price};
    if (order.side == Side::Buy) {
        bids_[order.price].push_back(std::move(order));
    } else {
        asks_[order.price].push_back(std::move(order));
    }
}

Order* OrderBook::find_order(OrderId order_id) {
    const auto location = order_index_.find(order_id);
    if (location == order_index_.end()) {
        return nullptr;
    }

    if (location->second.side == Side::Buy) {
        auto price_level = bids_.find(location->second.price);
        if (price_level == bids_.end()) {
            return nullptr;
        }
        for (auto& order : price_level->second) {
            if (order.id == order_id) {
                return &order;
            }
        }
        return nullptr;
    }

    auto price_level = asks_.find(location->second.price);
    if (price_level == asks_.end()) {
        return nullptr;
    }
    for (auto& order : price_level->second) {
        if (order.id == order_id) {
            return &order;
        }
    }
    return nullptr;
}

std::optional<Order> OrderBook::remove_order(OrderId order_id) {
    const auto location = order_index_.find(order_id);
    if (location == order_index_.end()) {
        return std::nullopt;
    }

    if (location->second.side == Side::Buy) {
        auto price_level = bids_.find(location->second.price);
        if (price_level == bids_.end()) {
            order_index_.erase(location);
            return std::nullopt;
        }
        auto& orders = price_level->second;
        const auto order_it = std::find_if(orders.begin(), orders.end(), [order_id](const Order& order) {
            return order.id == order_id;
        });
        if (order_it == orders.end()) {
            order_index_.erase(location);
            return std::nullopt;
        }
        Order removed = *order_it;
        orders.erase(order_it);
        if (orders.empty()) {
            bids_.erase(price_level);
        }
        order_index_.erase(location);
        return removed;
    }

    auto price_level = asks_.find(location->second.price);
    if (price_level == asks_.end()) {
        order_index_.erase(location);
        return std::nullopt;
    }
    auto& orders = price_level->second;
    const auto order_it = std::find_if(orders.begin(), orders.end(), [order_id](const Order& order) {
        return order.id == order_id;
    });
    if (order_it == orders.end()) {
        order_index_.erase(location);
        return std::nullopt;
    }
    Order removed = *order_it;
    orders.erase(order_it);
    if (orders.empty()) {
        asks_.erase(price_level);
    }
    order_index_.erase(location);
    return removed;
}

void OrderBook::match(Order& order, SubmitResult& result) {
    Quantity matched_quantity = 0;

    while (order.leaves_quantity > 0 && crosses(order)) {
        auto& resting_queue = best_opposite_queue(order.side);
        auto& resting = resting_queue.front();
        const Quantity fill_quantity = std::min(order.leaves_quantity, resting.leaves_quantity);
        const Price fill_price = resting.price;

        order.leaves_quantity -= fill_quantity;
        resting.leaves_quantity -= fill_quantity;
        matched_quantity += fill_quantity;

        result.trades.push_back(Trade{symbol_, order.id, resting.id, fill_price, fill_quantity});
        result.reports.push_back(make_report(order,
                                             order.leaves_quantity == 0 ? OrderStatus::Filled : OrderStatus::PartiallyFilled,
                                             fill_quantity,
                                             fill_price,
                                             "aggressor fill"));
        result.reports.push_back(make_report(resting,
                                             resting.leaves_quantity == 0 ? OrderStatus::Filled : OrderStatus::PartiallyFilled,
                                             fill_quantity,
                                             fill_price,
                                             "resting fill"));

        if (resting.leaves_quantity == 0) {
            order_index_.erase(resting.id);
            resting_queue.pop_front();
            erase_best_if_empty(order.side);
        }
    }

    if (order.leaves_quantity > 0) {
        add_resting_order(order);
        if (matched_quantity > 0) {
            result.reports.push_back(make_report(order, OrderStatus::PartiallyFilled, 0, 0, "resting remainder"));
        }
    }
}

ExecutionReport OrderBook::make_report(const Order& order, OrderStatus status, Quantity last_quantity, Price last_price, const std::string& text) const {
    const Quantity cumulative = order.quantity >= order.leaves_quantity ? order.quantity - order.leaves_quantity : 0;
    return ExecutionReport{order.id,
                           order.client_order_id,
                           order.symbol,
                           order.side,
                           status,
                           order.price,
                           last_price,
                           order.quantity,
                           last_quantity,
                           cumulative,
                           order.leaves_quantity,
                           text};
}

} // namespace ome
