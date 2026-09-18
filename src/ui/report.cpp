#include "report.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace quant::ui {

void print_banner(const std::string& strategy_name) {
    // TODO: a short header naming the strategy and setting up the run.
    (void)strategy_name;
}

void print_report(const metrics::PerformanceReport& report) {
    // TODO: print the report. You may as well just do `std::cout << report`:
    // that is what the overloaded operator in metrics.hpp exists for, and
    // re-formatting it here would defeat the point of putting it there.
    (void)report;
}

void print_equity_curve(const std::vector<double>& equity, int width, int height) {
    // TODO: draw it with block characters. A workable approach: map each of
    // the `height` rows to an equity band between the curve's min and max,
    // then for each of `width` columns decide whether the curve sits at or
    // above that band. Guard the flat-curve case where max == min and every
    // band would have zero height.
    (void)equity;
    (void)width;
    (void)height;
}

void print_monte_carlo(const MonteCarloResult& result) {
    // TODO: print the percentile bands, with a line saying what they mean --
    // a bare number labelled "p05" tells a reader nothing.
    (void)result;
}

}  // namespace quant::ui
