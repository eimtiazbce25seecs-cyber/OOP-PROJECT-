#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "event.hpp"

namespace quant {

/// A holding in a single symbol.
///
/// Composed into Portfolio below. It tracks average cost so the portfolio can
/// report unrealised P&L without having to remember every individual purchase.
class Position {
public:
    /// Add `quantity` shares acquired at `price`, folding them into the
    /// running average cost.
    void add(long quantity, double price);

    /// Remove `quantity` shares. Average cost is unchanged by a sale.
    void reduce(long quantity);

    long quantity() const noexcept;
    double average_cost() const noexcept;

    /// (mark - average_cost) * quantity.
    double unrealized_pnl(double mark) const noexcept;

    /// Current market value of the holding at `mark`.
    double market_value(double mark) const noexcept;

private:
    long quantity_{0};
    double average_cost_{0.0};
};

/// Cash, positions, and the equity history the metrics are computed from.
///
/// Every member is private. The only ways in are apply_fill() and
/// mark_to_market(), and both preserve the class invariants -- in particular
/// that cash never goes negative. Callers cannot reach in and break that.
class Portfolio {
public:
    /// Throws std::invalid_argument if initial_cash is negative.
    explicit Portfolio(double initial_cash);

    /// Apply an execution to the book.
    /// Returns false and changes nothing if the fill would overdraw the
    /// account, so the caller learns the order was rejected rather than having
    /// an exception thrown at it.
    bool apply_fill(const FillEvent& fill);

    /// Record the latest price for a symbol, so equity() can value the book.
    void mark_to_market(const std::string& symbol, double price);

    /// Append the current equity to the curve. Call once per bar, after
    /// mark_to_market.
    void record_equity();

    double cash() const noexcept;
    /// Cash plus the marked value of every open position.
    double equity() const noexcept;
    const std::vector<double>& equity_curve() const noexcept;
    std::size_t num_trades() const noexcept;

private:
    double cash_;
    std::map<std::string, Position> positions_;
    std::map<std::string, double> last_price_;
    std::vector<double> equity_curve_;
    std::size_t num_trades_{0};
};

}  // namespace quant
