#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "csv_parser.hpp"
#include "event.hpp"
#include "metrics.hpp"
#include "monte_carlo.hpp"
#include "strategy.hpp"

#include "engine/backtest.hpp"
#include "ui/report.hpp"

namespace {

/// The data lives in data/, but the executable runs from build/ just as often
/// as from the project root. Try the sibling directory before giving up.
std::string resolve_data_path(const std::string& given) {
    if (std::ifstream(given).good()) {
        return given;
    }
    const std::string sibling = "../" + given;
    if (std::ifstream(sibling).good()) {
        return sibling;
    }
    return given;  // let the parser raise the error, quoting the original path
}

}  // namespace

int main(int argc, char** argv) {
    const std::string strategy_name = (argc > 1) ? argv[1] : "sma";
    const std::string csv_path =
        resolve_data_path((argc > 2) ? argv[2] : "data/AAPL.csv");

    try {
        quant::ui::print_banner(strategy_name);
        std::cout << "[scaffold] function bodies are TODO -- grep for TODO under src/\n\n";

        quant::CsvParser parser;
        const std::vector<quant::Bar> bars = parser.parse_file(csv_path);
        std::cout << "loaded " << bars.size() << " bars from " << csv_path << "\n";

        quant::BacktestConfig config;
        config.symbol = "AAPL";

        quant::Backtest backtest(config, quant::make_strategy(strategy_name));
        const quant::metrics::PerformanceReport report = backtest.run(bars);
        quant::ui::print_report(report);

        const std::vector<double>& equity = backtest.equity_curve();
        quant::ui::print_equity_curve(equity, 64, 12);

        if (equity.size() > 1) {
            // Seeded on purpose: a Monte Carlo run you cannot reproduce is a
            // number you cannot check.
            quant::MonteCarloSimulator simulator(12345);
            const std::vector<double> returns = quant::metrics::to_returns(equity);
            const quant::MonteCarloResult result =
                simulator.bootstrap(returns, 1000, 252, equity.back());
            quant::ui::print_monte_carlo(result);
        }
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
