#pragma once

#include <memory>
#include <string>
#include <vector>

#include "event.hpp"
#include "metrics.hpp"
#include "portfolio.hpp"
#include "strategy.hpp"

namespace quant {

/// Tunable knobs for a single backtest run.
struct BacktestConfig {
    std::string symbol{"AAPL"};
    double initial_cash{100'000.0};
    double commission{0.001};   ///< fraction of notional: 0.001 is 10 basis points
    long order_size{100};       ///< shares per order
};

/// Drives the event loop: bars go in, a performance report comes out.
///
/// This is where the other six headers meet. It owns its strategy through a
/// unique_ptr<Strategy>, so the concrete type is decided at runtime by
/// make_strategy() and this class never names BuyAndHold or SmaCrossover --
/// adding a strategy does not touch this file.
class Backtest {
public:
    Backtest(BacktestConfig config, std::unique_ptr<Strategy> strategy);

    /// Replay `bars` and return the resulting metrics.
    /// Resets the portfolio, strategy and queue first, so the same Backtest
    /// object can be run more than once.
    metrics::PerformanceReport run(const std::vector<Bar>& bars);

    /// Equity after each bar, for the UI to plot. Meaningful once run() has
    /// been called, empty before that.
    const std::vector<double>& equity_curve() const noexcept;

private:
    // Each stage converts one event type into the next. Private because they
    // are internal steps of run(), not part of what a user of Backtest needs.
    void handle_market(const MarketEvent& event);
    void handle_signal(const SignalEvent& event);
    void handle_order(const OrderEvent& event);
    void handle_fill(const FillEvent& event);

    BacktestConfig config_;
    std::unique_ptr<Strategy> strategy_;
    Portfolio portfolio_;
    EventQueue queue_;
    /// Realised P&L for each round trip, kept so metrics can compute win_rate.
    std::vector<double> trade_pnls_;
    /// Open position cost basis, so a sale can be matched against its purchase.
    double open_cost_basis_{0.0};
};

}  // namespace quant
