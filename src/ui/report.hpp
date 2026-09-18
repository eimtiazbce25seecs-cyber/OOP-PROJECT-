#pragma once

#include <string>
#include <vector>

#include "metrics.hpp"
#include "monte_carlo.hpp"

namespace quant::ui {

/// Header line naming the strategy about to run.
void print_banner(const std::string& strategy_name);

/// The performance table.
void print_report(const metrics::PerformanceReport& report);

/// Draw the equity curve as a block-character chart, `width` columns wide and
/// `height` rows tall. Deliberately ASCII so the project needs no plot library.
void print_equity_curve(const std::vector<double>& equity, int width, int height);

/// Print the percentile bands from a Monte Carlo run.
void print_monte_carlo(const MonteCarloResult& result);

}  // namespace quant::ui
