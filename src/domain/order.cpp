#include "ome/domain/order.hpp"

namespace ome {

std::string to_string(Side side) {
    return side == Side::Buy ? "BUY" : "SELL";
}

std::string to_string(OrderStatus status) {
    switch (status) {
    case OrderStatus::Accepted:
        return "ACCEPTED";
    case OrderStatus::PartiallyFilled:
        return "PARTIALLY_FILLED";
    case OrderStatus::Filled:
        return "FILLED";
    case OrderStatus::Replaced:
        return "REPLACED";
    case OrderStatus::Canceled:
        return "CANCELED";
    case OrderStatus::Rejected:
        return "REJECTED";
    }
    return "UNKNOWN";
}

} // namespace ome
