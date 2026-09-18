#pragma once

#include <memory>
#include <queue>
#include <string>

namespace quant {

/// What kind of payload an Event is actually carrying.
enum class EventType { Market, Signal, Order, Fill };

/// Which side of the book an order or fill acts on.
enum class Direction { Buy, Sell };

/// "BUY" / "SELL" -- used by the describe() implementations.
const char* to_string(Direction direction) noexcept;

/// One OHLCV row of market data.
///
/// Declared here rather than in csv_parser.hpp so the dependency runs one way
/// only: the parser reads Bars, the events carry them. Nothing in this header
/// needs to know that CSV files exist.
struct Bar {
    std::string date;
    double open{0.0};
    double high{0.0};
    double low{0.0};
    double close{0.0};
    long volume{0};
};

/// Abstract base for everything that travels through the engine's queue.
///
/// The queue owns events as std::unique_ptr<Event>, so every event is destroyed
/// through a base-class pointer. That is exactly why ~Event() has to be virtual:
/// without it, a MarketEvent's std::string member would never be destroyed.
class Event {
public:
    explicit Event(EventType type) noexcept;
    virtual ~Event() = default;

    Event(const Event&) = default;
    Event& operator=(const Event&) = default;

    EventType type() const noexcept;

    /// One-line summary for logging. Pure virtual, which makes Event abstract:
    /// you cannot construct a plain Event, only one of the four below.
    virtual std::string describe() const = 0;

private:
    EventType type_;
};

/// A new bar has arrived. Emitted by the engine, consumed by the strategy.
class MarketEvent : public Event {
public:
    explicit MarketEvent(Bar bar);

    const Bar& bar() const noexcept;

    std::string describe() const override;

private:
    Bar bar_;
};

/// A strategy wants to act. Produced by Strategy::on_bar().
class SignalEvent : public Event {
public:
    SignalEvent(std::string symbol, Direction direction, double strength);

    const std::string& symbol() const noexcept;
    Direction direction() const noexcept;
    /// Conviction in [0, 1]. 0 means the strategy is unsure; the engine may
    /// still act, but the field is there for sizing rules later.
    double strength() const noexcept;

    std::string describe() const override;

private:
    std::string symbol_;
    Direction direction_;
    double strength_;
};

/// A sized, actionable instruction. Produced from a SignalEvent by the engine,
/// which is the layer that knows the account size and the order-size policy.
class OrderEvent : public Event {
public:
    OrderEvent(std::string symbol, Direction direction, long quantity);

    const std::string& symbol() const noexcept;
    Direction direction() const noexcept;
    long quantity() const noexcept;

    std::string describe() const override;

private:
    std::string symbol_;
    Direction direction_;
    long quantity_;
};

/// An order that actually executed: at a price, for a cost.
class FillEvent : public Event {
public:
    FillEvent(std::string symbol, Direction direction, long quantity,
              double price, double commission);

    const std::string& symbol() const noexcept;
    Direction direction() const noexcept;
    long quantity() const noexcept;
    double price() const noexcept;
    double commission() const noexcept;

    /// Signed cash impact including commission: negative when buying
    /// (cash leaves the account), positive when selling.
    double cash_flow() const noexcept;

    std::string describe() const override;

private:
    std::string symbol_;
    Direction direction_;
    long quantity_;
    double price_;
    double commission_;
};

/// The engine's event queue. unique_ptr rather than a value type because Event
/// is abstract and slicing a MarketEvent into an Event would lose the payload.
using EventQueue = std::queue<std::unique_ptr<Event>>;

}  // namespace quant
