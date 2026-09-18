#include "metrics.hpp"

#include <cstddef>
#include <ostream>
#include <vector>

namespace quant::metrics {

std::ostream& operator<<(std::ostream& os, const PerformanceReport& report) {
    // TODO: format the report. Percentages with a fixed precision read far
    // better than raw fractions -- "15.00%" beats "0.15".
    // Keeping the formatting here rather than in ui/ is the point: the UI
    // layer just does `std::cout << report`.
    (void)report;
    return os;
}

double total_return(const std::vector<double>& equity) {
    // TODO: (final - initial) / initial. Decide what an empty or
    // single-element curve should return, and guard the divide.
    (void)equity;
    return 0.0;
}

double cagr(const std::vector<double>& equity, double years) {
    // TODO: (final / initial)^(1 / years) - 1. Note the difference from
    // total_return: this is the *annualised* rate, so a 2-year run that
    // doubled has a CAGR well under 100%.
    (void)equity;
    (void)years;
    return 0.0;
}

double volatility(const std::vector<double>& returns, int periods_per_year) {
    // TODO: standard deviation of the periodic returns, scaled to a year by
    // sqrt(periods_per_year). quant::stats::stdev is in math/statistics.hpp.
    // Scaling by the square root, not by the period count itself, is the
    // detail that catches people out.
    (void)returns;
    (void)periods_per_year;
    return 0.0;
}

double sharpe(const std::vector<double>& returns, double risk_free_rate,
              int periods_per_year) {
    // TODO: (mean excess return / stdev of returns) * sqrt(periods_per_year).
    // Guard the case where stdev is zero -- a flat curve has no risk to
    // divide by.
    (void)returns;
    (void)risk_free_rate;
    (void)periods_per_year;
    return 0.0;
}

double max_drawdown(const std::vector<double>& equity) {
    // TODO: track the running peak and the largest (peak - value) / peak.
    // Return it as a POSITIVE fraction -- the header promises 0.20 for a 20%
    // decline, so the sign convention is yours to get right once, here.
    (void)equity;
    return 0.0;
}

double win_rate(const std::vector<double>& trade_pnls) {
    // TODO: fraction of trade P&Ls that are positive. Decide whether a
    // break-even trade counts, and be consistent about it.
    (void)trade_pnls;
    return 0.0;
}

std::vector<double> to_returns(const std::vector<double>& equity) {
    // TODO: r[i] = (equity[i] - equity[i-1]) / equity[i-1], so the result has
    // one fewer element than its input. That off-by-one is normal but worth
    // being deliberate about.
    (void)equity;
    return {};
}

PerformanceReport summarize(const std::vector<double>& equity,
                            const std::vector<double>& trade_pnls,
                            int periods_per_year) {
    // TODO: call each metric above exactly once and fill in the struct.
    // The equity curve is a year of daily bars, so pass periods_per_year
    // through to the annualisers and use 1.0 for `years` (or derive it from
    // the bar count divided by periods_per_year, which is more honest).
    (void)equity;
    (void)trade_pnls;
    (void)periods_per_year;
    return {};
}

}  // namespace quant::metrics
