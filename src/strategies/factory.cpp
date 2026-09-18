#include "strategy.hpp"

#include <memory>
#include <stdexcept>
#include <string>

namespace quant {

std::unique_ptr<Strategy> make_strategy(const std::string& name) {
    // TODO: map a name to a concrete strategy and return it:
    //     "buyhold" -> BuyAndHoldStrategy
    //     "sma"     -> SmaCrossoverStrategy(10, 30)
    //     "meanrev" -> MeanReversionStrategy(20, 0.02)
    //     anything else -> throw std::invalid_argument listing the valid names
    //
    // Returning unique_ptr<Strategy> by value is the whole trick: the caller
    // ends up owning a concrete type it never has to name, which is what lets
    // Backtest stay ignorant of every class in strategies/.
    (void)name;
    throw std::invalid_argument("make_strategy: not implemented yet");
}

}  // namespace quant
