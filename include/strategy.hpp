#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "event.hpp"

namespace quant {

/// What a strategy wants to do next.
struct Signal {
    Direction direction{Direction::Buy};
    /// Conviction in [0, 1].
    double strength{0.0};
};

/// Abstract trading strategy.
///
/// The engine holds a unique_ptr<Strategy> and calls on_bar() through it, so a
/// concrete strategy is chosen at runtime (by make_strategy) and the engine
/// never names any of the three classes below. That indirection is the point of
/// the hierarchy: adding a fourth strategy means adding a .cpp file, not editing
/// the engine.
class Strategy {
public:
    virtual ~Strategy() = default;

    /// Identifier used by the factory and the UI. Pure virtual.
    virtual std::string name() const = 0;

    /// Called once per bar. Return std::nullopt to do nothing this bar.
    /// Implementations keep their own price history as private members, which is
    /// why this takes only the newest bar.
    virtual std::optional<Signal> on_bar(const Bar& bar) = 0;

    /// Discard accumulated state so the strategy can be replayed.
    /// Declared non-pure on purpose: a stateless strategy inherits a working
    /// default instead of being forced to write an empty override.
    virtual void reset() {}
};

/// Buy once on the first bar and hold to the end. The baseline every other
/// strategy is measured against.
class BuyAndHoldStrategy : public Strategy {
public:
    std::string name() const override;
    std::optional<Signal> on_bar(const Bar& bar) override;
    void reset() override;

private:
    bool bought_{false};
};

/// Long while the fast moving average is above the slow one, flat otherwise.
class SmaCrossoverStrategy : public Strategy {
public:
    /// Throws std::invalid_argument unless 0 < short_window < long_window.
    SmaCrossoverStrategy(int short_window, int long_window);

    std::string name() const override;
    std::optional<Signal> on_bar(const Bar& bar) override;
    void reset() override;

private:
    int short_window_;
    int long_window_;
    /// Rolling window of closes seen so far, oldest first. Private: the engine
    /// never sees it, and no caller can corrupt it.
    std::vector<double> closes_;
    bool in_position_{false};
};

/// Contrarian: buy when price falls `threshold` below its rolling mean, sell
/// when it rises the same distance above.
class MeanReversionStrategy : public Strategy {
public:
    /// `threshold` is a fraction, e.g. 0.02 for 2%.
    MeanReversionStrategy(int lookback, double threshold);

    std::string name() const override;
    std::optional<Signal> on_bar(const Bar& bar) override;
    void reset() override;

private:
    int lookback_;
    double threshold_;
    std::vector<double> closes_;
    bool in_position_{false};
};

/// Build a strategy by name: "buyhold", "sma", or "meanrev".
/// Throws std::invalid_argument for an unknown name.
std::unique_ptr<Strategy> make_strategy(const std::string& name);

}  // namespace quant
