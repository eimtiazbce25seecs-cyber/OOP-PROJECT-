// Entry point.
//
// Two front ends over the same engine: an ImGui window by default, and the
// original ASCII report behind --nogui. The console path is not vestigial --
// it is the fallback whenever a window cannot be created, which is exactly the
// situation on a machine with no display or no usable OpenGL driver.

#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// csv_parser.hpp lives in include/, which is already on the include path via
// target_include_directories(quant PUBLIC include) -- so the plain quoted form
// below is all that is needed, and it is the correct form.
//
// Do not replace this with a __has_include() chain searching data/, ../data/ or
// src/data/. The header is in none of those directories, so such a chain can
// never find it -- and because a quoted include searches the *including file's*
// own directory first, it would silently pick up any stray src/data/csv_parser.hpp
// instead, giving "'CsvParser' is not a member of 'quant'" in this file and in
// src/data/csv_parser.cpp.
#include "csv_parser.hpp"
#include "event.hpp"
#include "metrics.hpp"
#include "monte_carlo.hpp"
#include "strategy.hpp"

#include "engine/backtest.hpp"
#include "ui/gui.hpp"
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

void print_usage(const char* program) {
    std::cout << "usage: " << program << " [--nogui] [strategy] [csv-path]\n"
              << "\n"
              << "  --nogui     skip the window and print the ASCII report\n"
              << "  strategy    buy_and_hold | sma | mean_reversion  (default: sma)\n"
              << "  csv-path    OHLCV csv to load      (default: data/AAPL.csv)\n";
}

/// The original console front end: load, backtest, print.
int run_console(const std::string& strategy_name, const std::string& csv_path) {
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

}  // namespace

int main(int argc, char** argv) {
    bool want_gui = true;
    std::vector<std::string> positional;

    // Flags are recognised anywhere, so "main sma --nogui" and
    // "main --nogui sma" behave the same.
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--nogui") {
            want_gui = false;
        } else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else {
            positional.push_back(arg);
        }
    }

    const std::string strategy_name = positional.empty() ? "sma" : positional[0];
    const std::string csv_path = resolve_data_path(
        positional.size() > 1 ? positional[1] : "data/AAPL.csv");

    if (want_gui) {
        try {
            quant::ui::GuiConfig config;
            config.data_path = csv_path;
            config.strategy = strategy_name;

            quant::ui::GuiApp app(config);
            return app.run();
        } catch (const std::exception& error) {
            // glfwInit failed, or no GL 3.3 window could be created. That is a
            // normal outcome on a headless machine, not a bug, so report it and
            // do the useful thing instead of exiting non-zero.
            std::cerr << "gui unavailable: " << error.what() << '\n'
                      << "falling back to the console report\n\n";
        }
    }

    return run_console(strategy_name, csv_path);
}
