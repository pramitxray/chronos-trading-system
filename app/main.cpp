#include "ome/engine/matching_engine.hpp"
#include "ome/fix/fix_parser.hpp"

#include <fstream>
#include <iostream>
#include <set>

namespace {

void print_report(const ome::ExecutionReport& report) {
    std::cout << "EXEC_REPORT"
              << " order_id=" << report.order_id
              << " cl_ord_id=" << report.client_order_id
              << " symbol=" << report.symbol
              << " side=" << ome::to_string(report.side)
              << " status=" << ome::to_string(report.status)
              << " last_qty=" << report.last_quantity
              << " last_px=" << report.last_price
              << " cum_qty=" << report.cumulative_quantity
              << " leaves=" << report.leaves_quantity
              << " text=\"" << report.text << "\"\n";
}

void print_trade(const ome::Trade& trade) {
    std::cout << "TRADE"
              << " symbol=" << trade.symbol
              << " aggressor=" << trade.aggressor_order_id
              << " resting=" << trade.resting_order_id
              << " qty=" << trade.quantity
              << " px=" << trade.price << "\n";
}

void print_snapshot(const ome::BookSnapshot& snapshot) {
    std::cout << "BOOK " << snapshot.symbol << "\n";
    std::cout << "  BIDS:";
    for (const auto& level : snapshot.bids) {
        std::cout << " " << level.price << "x" << level.quantity;
    }
    std::cout << "\n  ASKS:";
    for (const auto& level : snapshot.asks) {
        std::cout << " " << level.price << "x" << level.quantity;
    }
    std::cout << "\n";
}

} // namespace

int main(int argc, char** argv) {
    const std::string path = argc > 1 ? argv[1] : "samples/orders.fix";
    std::ifstream input(path);
    if (!input) {
        std::cerr << "could not open input file: " << path << "\n";
        return 1;
    }

    ome::MatchingEngine engine;
    std::set<std::string> touched_symbols;
    std::string line;
    std::size_t line_number = 0;

    while (std::getline(input, line)) {
        ++line_number;
        std::string error;
        const auto message = ome::FixParser::parse(line, error);
        if (!message.has_value()) {
            if (!error.empty()) {
                std::cerr << "line " << line_number << ": " << error << "\n";
            }
            continue;
        }

        if (!message->symbol.empty()) {
            touched_symbols.insert(message->symbol);
        }

        auto result = engine.submit(*message);
        for (const auto& trade : result.trades) {
            print_trade(trade);
        }
        for (const auto& report : result.reports) {
            print_report(report);
        }
    }

    std::cout << "\nFINAL SNAPSHOTS\n";
    for (const auto& symbol : touched_symbols) {
        print_snapshot(engine.snapshot(symbol));
    }

    return 0;
}
