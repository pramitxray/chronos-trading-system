#pragma once

#include "order.hpp"

#include <optional>
#include <string>
#include <unordered_map>

namespace ome {

enum class FixMessageType {
    NewOrderSingle,
    CancelRequest
};

struct FixOrderMessage {
    FixMessageType type{FixMessageType::NewOrderSingle};
    std::string client_order_id;
    std::string original_client_order_id;
    std::string symbol;
    Side side{Side::Buy};
    Quantity quantity{};
    Price price{};
};

class FixParser {
public:
    static std::optional<FixOrderMessage> parse(const std::string& line, std::string& error);

private:
    static std::unordered_map<std::string, std::string> parse_fields(const std::string& line);
};

} // namespace ome
