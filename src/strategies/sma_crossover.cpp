#include "strategy.hpp"

#include <string>

namespace quant {

SmaCrossoverStrategy::SmaCrossoverStrategy(int short_window, int long_window)
    : short_window_(short_window), long_window_(long_window) {
    // TODO: validate before anything else -- throw std::invalid_argument
    // unless 0 < short_window < long_window. Without this, a nonsense
    // configuration runs silently to completion and reports a flat zero,
    // which is much harder to debug than an exception at construction.
}

std::string SmaCrossoverStrategy::name() const {
    // TODO: include the windows, e.g. "sma(10,30)" -- the UI prints this, and
    // two runs with different windows should not look identical.
    return "sma";
}

std::optional<Signal> SmaCrossoverStrategy::on_bar(const Bar& bar) {
    // TODO: push bar.close onto closes_. While you have fewer than
    // long_window_ closes, return nullopt -- the slow average is not defined
    // yet, and guessing would produce a signal you cannot justify.
    //
    // Once you have enough history, compare the short mean against the long
    // mean (quant::stats::mean is in math/statistics.hpp):
    //   - short above long while flat    -> Buy,  set in_position_
    //   - short below long while long    -> Sell, clear in_position_
    //   - already positioned the right way -> nullopt, do not re-buy every bar
    //
    // Trimming closes_ to the last long_window_ entries is up to you: letting
    // it grow unbounded still gives correct answers, just less tidily.
    (void)bar;
    return std::nullopt;
}

void SmaCrossoverStrategy::reset() {
    // TODO: clear every piece of accumulated state: the price history and the
    // position flag. Miss one and a second run() starts from a stale world.
}

}  // namespace quant
