#include "strategy.hpp"

#include <string>

namespace quant {

MeanReversionStrategy::MeanReversionStrategy(int lookback, double threshold)
    : lookback_(lookback), threshold_(threshold) {
    // TODO: validate -- lookback must be positive and threshold must be
    // positive. Throw std::invalid_argument otherwise.
}

std::string MeanReversionStrategy::name() const {
    // TODO: include the parameters, e.g. "meanrev(20,0.02)".
    return "meanrev";
}

std::optional<Signal> MeanReversionStrategy::on_bar(const Bar& bar) {
    // TODO: this is the mirror image of the crossover -- it fades moves
    // instead of following them.
    //   - accumulate closes_, and wait until you have lookback_ of them
    //   - compute the rolling mean
    //   - if close is more than `threshold_` BELOW the mean and you are flat,
    //     Buy (the price looks cheap)
    //   - if close is more than `threshold_` ABOVE the mean and you are long,
    //     Sell (take the profit)
    // Shop around the edge cases: exactly at the threshold, and what happens
    // on the first bar where the window is only just full.
    (void)bar;
    return std::nullopt;
}

void MeanReversionStrategy::reset() {
    // TODO: clear the accumulated state, as with the other strategies.
}

}  // namespace quant
