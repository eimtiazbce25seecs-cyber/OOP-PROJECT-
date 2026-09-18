#pragma once

#include <cstddef>
#include <iosfwd>
#include <vector>

namespace quant::metrics {

/// Everything the UI needs to print a run summary.
///
/// A plain aggregate with public members and no methods: metrics are computed,
/// not maintained, so there is no invariant to protect. Contrast with
/// Portfolio, where encapsulation is doing real work.
struct PerformanceReport {
    double total_return{0.0};   ///< 0.15 means +15%
    double cagr{0.0};           ///< compound annual growth rate
    double sharpe{0.0};         ///< annualised
    double volatility{0.0};     ///< annualised standard deviation
    double max_drawdown{0.0};   ///< positive fraction: 0.20 means -20% peak to trough
    double win_rate{0.0};       ///< fraction of round-trip trades that made money
    std::size_t num_trades{0};
};

/// Formatted multi-line summary. Operator overloading keeps the formatting
/// inside the metrics layer, so ui/ just does `std::cout << report`.
std::ostream& operator<<(std::ostream& os, const PerformanceReport& report);

/// (final - initial) / initial.
double total_return(const std::vector<double>& equity);

/// Compound annual growth rate. `years` is the length of the sample.
double cagr(const std::vector<double>& equity, double years);

/// Annualised standard deviation of periodic returns.
double volatility(const std::vector<double>& returns, int periods_per_year);

/// Annualised Sharpe ratio: excess return per unit of risk.
double sharpe(const std::vector<double>& returns, double risk_free_rate,
              int periods_per_year);

/// Largest peak-to-trough decline, returned as a positive fraction.
double max_drawdown(const std::vector<double>& equity);

/// Fraction of trade P&Ls that were positive.
double win_rate(const std::vector<double>& trade_pnls);

/// Convert an equity curve into period-over-period simple returns.
std::vector<double> to_returns(const std::vector<double>& equity);

/// Run every metric above and bundle the results.
/// `periods_per_year` is 252 for daily bars.
PerformanceReport summarize(const std::vector<double>& equity,
                            const std::vector<double>& trade_pnls,
                            int periods_per_year = 252);

}  // namespace quant::metrics
