#include "strategy.hpp"

#include <string>

namespace quant {

std::string BuyAndHoldStrategy::name() const { return "buyhold"; }

std::optional<Signal> BuyAndHoldStrategy::on_bar(const Bar& bar) {
    // TODO: return a Buy signal the first time this is called, then nullopt
    // forever after. `bought_` is the latch that makes "once" mean once --
    // and because it is private, nothing outside this class can flip it back.
    (void)bar;
    return std::nullopt;
}

void BuyAndHoldStrategy::reset() {
    // TODO: restore the initial state so the strategy can be replayed.
    // Ask yourself what "initial state" means for a one-shot strategy.
}

}  // namespace quant
