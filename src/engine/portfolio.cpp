#include "portfolio.hpp"

#include <stdexcept>

namespace quant {

// ---------------------------------------------------------------------------
// Position
// ---------------------------------------------------------------------------

void Position::add(long quantity, double price) {
    // TODO: fold the new shares into the running average cost:
    //     avg_cost_ = (avg_cost_ * quantity_ + price * quantity)
    //                 / (quantity_ + quantity)
    // Guard against a zero or negative `quantity` before you divide by it.
    (void)quantity;
    (void)price;
}

void Position::reduce(long quantity) {
    // TODO: subtract from quantity_. A sale does not change average cost --
    // decide what should happen if `quantity` exceeds what is held.
    (void)quantity;
}

long Position::quantity() const noexcept { return quantity_; }

double Position::average_cost() const noexcept { return average_cost_; }

double Position::unrealized_pnl(double mark) const noexcept {
    // TODO: (mark - average_cost_) * quantity_
    (void)mark;
    return 0.0;
}

double Position::market_value(double mark) const noexcept {
    // TODO: mark * quantity_
    (void)mark;
    return 0.0;
}

// ---------------------------------------------------------------------------
// Portfolio
// ---------------------------------------------------------------------------

Portfolio::Portfolio(double initial_cash) : cash_(initial_cash) {
    // TODO: reject nonsense up front -- throw std::invalid_argument if
    // initial_cash is negative. Failing at construction beats discovering a
    // broken portfolio halfway through a run.
}

bool Portfolio::apply_fill(const FillEvent& fill) {
    // TODO: this is the method that enforces the class invariant.
    //   1. work out the cash required: quantity * price + commission
    //   2. if this is a buy and cash_ is short, return false and change
    //      NOTHING -- no partial state
    //   3. otherwise move cash_ by fill.cash_flow() and update the Position
    //      for fill.symbol(): add on a buy, reduce on a sell
    //   4. ++num_trades_
    (void)fill;
    return false;
}

void Portfolio::mark_to_market(const std::string& symbol, double price) {
    // TODO: record the latest price. equity() needs this to value open
    // positions, and without it every position looks worthless.
    (void)symbol;
    (void)price;
}

void Portfolio::record_equity() {
    // TODO: append equity() to equity_curve_. Call once per bar.
}

double Portfolio::cash() const noexcept { return cash_; }

double Portfolio::equity() const noexcept {
    // TODO: cash_ plus the marked value of every open position.
    return cash_;
}

const std::vector<double>& Portfolio::equity_curve() const noexcept {
    return equity_curve_;
}

std::size_t Portfolio::num_trades() const noexcept { return num_trades_; }

}  // namespace quant
