#include "event.hpp"

#include <string>
#include <utility>

namespace quant {

const char* to_string(Direction direction) noexcept {
    // TODO: Direction::Buy -> "BUY", Direction::Sell -> "SELL".
    // Think about why the fallthrough here is `return "?"` rather than a throw:
    // this function is noexcept, and it is used while formatting a log line.
    (void)direction;
    return "?";
}

Event::Event(EventType type) noexcept : type_(type) {}

EventType Event::type() const noexcept { return type_; }

// ---------------------------------------------------------------------------
// MarketEvent
// ---------------------------------------------------------------------------

MarketEvent::MarketEvent(Bar bar) : Event(EventType::Market), bar_(std::move(bar)) {}

const Bar& MarketEvent::bar() const noexcept { return bar_; }

std::string MarketEvent::describe() const {
    // TODO: e.g. "MARKET   2024-01-02  close=150.25"
    return {};
}

// ---------------------------------------------------------------------------
// SignalEvent
// ---------------------------------------------------------------------------

SignalEvent::SignalEvent(std::string symbol, Direction direction, double strength)
    : Event(EventType::Signal),
      symbol_(std::move(symbol)),
      direction_(direction),
      strength_(strength) {}

const std::string& SignalEvent::symbol() const noexcept { return symbol_; }

Direction SignalEvent::direction() const noexcept { return direction_; }

double SignalEvent::strength() const noexcept { return strength_; }

std::string SignalEvent::describe() const {
    // TODO: e.g. "SIGNAL   AAPL BUY strength=1.00"
    return {};
}

// ---------------------------------------------------------------------------
// OrderEvent
// ---------------------------------------------------------------------------

OrderEvent::OrderEvent(std::string symbol, Direction direction, long quantity)
    : Event(EventType::Order),
      symbol_(std::move(symbol)),
      direction_(direction),
      quantity_(quantity) {}

const std::string& OrderEvent::symbol() const noexcept { return symbol_; }

Direction OrderEvent::direction() const noexcept { return direction_; }

long OrderEvent::quantity() const noexcept { return quantity_; }

std::string OrderEvent::describe() const {
    // TODO: e.g. "ORDER    AAPL BUY 100"
    return {};
}

// ---------------------------------------------------------------------------
// FillEvent
// ---------------------------------------------------------------------------

FillEvent::FillEvent(std::string symbol, Direction direction, long quantity,
                     double price, double commission)
    : Event(EventType::Fill),
      symbol_(std::move(symbol)),
      direction_(direction),
      quantity_(quantity),
      price_(price),
      commission_(commission) {}

const std::string& FillEvent::symbol() const noexcept { return symbol_; }

Direction FillEvent::direction() const noexcept { return direction_; }

long FillEvent::quantity() const noexcept { return quantity_; }

double FillEvent::price() const noexcept { return price_; }

double FillEvent::commission() const noexcept { return commission_; }

double FillEvent::cash_flow() const noexcept {
    // TODO: buying takes cash out of the account, selling puts it in, and the
    // commission always costs you. Get the sign right -- Portfolio::apply_fill
    // depends on this convention.
    return 0.0;
}

std::string FillEvent::describe() const {
    // TODO: e.g. "FILL     AAPL BUY 100 @ 150.25 commission=15.03"
    return {};
}

}  // namespace quant
