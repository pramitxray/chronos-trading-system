#include "fix_parser.hpp"

#include <sstream>
#include <stdexcept>

namespace ome {

std::optional<FixOrderMessage> FixParser::parse(const std::string& line, std::string& error) {
    error.clear();
    if (line.empty() || line[0] == '#') {
        return std::nullopt;
    }

    const auto fields = parse_fields(line);
    const auto type_it = fields.find("35");
    if (type_it == fields.end()) {
        error = "missing MsgType tag 35";
        return std::nullopt;
    }

    try {
        FixOrderMessage message;
        if (type_it->second == "D") {
            message.type = FixMessageType::NewOrderSingle;
            message.client_order_id = fields.at("11");
            message.symbol = fields.at("55");
            message.side = fields.at("54") == "1" ? Side::Buy : Side::Sell;
            message.quantity = static_cast<Quantity>(std::stoull(fields.at("38")));
            message.price = static_cast<Price>(std::stoll(fields.at("44")));
        } else if (type_it->second == "F") {
            message.type = FixMessageType::CancelRequest;
            message.original_client_order_id = fields.at("41");
        } else if (type_it->second == "G") {
            message.type = FixMessageType::AmendRequest;
            message.client_order_id = fields.at("11");
            message.original_client_order_id = fields.at("41");
            message.symbol = fields.at("55");
            message.side = fields.at("54") == "1" ? Side::Buy : Side::Sell;
            message.quantity = static_cast<Quantity>(std::stoull(fields.at("38")));
            message.price = static_cast<Price>(std::stoll(fields.at("44")));
        } else {
            error = "unsupported MsgType " + type_it->second;
            return std::nullopt;
        }
        return message;
    } catch (const std::out_of_range&) {
        error = "required FIX tag is missing";
    } catch (const std::invalid_argument&) {
        error = "numeric FIX tag has invalid value";
    }

    return std::nullopt;
}

std::unordered_map<std::string, std::string> FixParser::parse_fields(const std::string& line) {
    std::unordered_map<std::string, std::string> fields;
    std::stringstream stream(line);
    std::string token;

    while (std::getline(stream, token, '|')) {
        const auto separator = token.find('=');
        if (separator == std::string::npos) {
            continue;
        }
        fields[token.substr(0, separator)] = token.substr(separator + 1);
    }

    return fields;
}

} // namespace ome
