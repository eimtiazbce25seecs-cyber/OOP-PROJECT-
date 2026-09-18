#include "backtest.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace quant {

Backtest::Backtest(BacktestConfig config, std::unique_ptr<Strategy> strategy)
    : config_(std::move(config)),
      strategy_(std::move(strategy)),
      // config_ is declared above portfolio_, so it is already initialised by
      // the time we get here. Reading the *parameter* `config` instead would
      // read a moved-from object -- the member-initialiser-order trap.
      portfolio_(config_.initial_cash) {}

metrics::PerformanceReport Backtest::run(const std::vector<Bar>& bars) {
    // TODO: the loop that everything else in this project exists to serve.
    // For each bar:
    //   1. push a MarketEvent onto queue_
    //   2. drain the queue until empty, dispatching on event->type() to the
    //      four handlers below. This switch is the ONLY place in the project
    //      that should care which concrete type an Event is.
    //   3. portfolio_.record_equity(), so the curve gets one point per bar
    //
    // Reset the portfolio, the strategy and the queue first, so calling run()
    // twice gives the same answer twice.
    //
    // Then return metrics::summarize(portfolio_.equity_curve(), trade_pnls_).
    (void)bars;
    return {};
}

const std::vector<double>& Backtest::equity_curve() const noexcept {
    return portfolio_.equity_curve();
}

void Backtest::handle_market(const MarketEvent& event) {
    // TODO: mark the portfolio to the bar's close, then ask strategy_ for a
    // signal. on_bar returns std::optional<Signal>, so only build a
    // SignalEvent when it actually holds a value.
    //
    // This call is the payoff of the whole hierarchy: the concrete type was
    // chosen by make_strategy(), and this line neither knows nor cares which
    // one it got.
    (void)event;
}

void Backtest::handle_signal(const SignalEvent& event) {
    // TODO: turn a signal into a sized OrderEvent, using config_.order_size.
    // This is the layer that knows how big an order should be -- the strategy
    // only expresses a direction.
    (void)event;
}

void Backtest::handle_order(const OrderEvent& event) {
    // TODO: simulate the execution. Decide a fill price (the last close is
    // the simplest defensible choice) and a commission from config_.commission,
    // then emit a FillEvent onto the queue.
    (void)event;
}

void Backtest::handle_fill(const FillEvent& event) {
    // TODO: hand the fill to the portfolio. apply_fill returns false when the
    // order would overdraw the account -- decide what a rejected fill means
    // here, because silently dropping it hides a strategy that is trying to
    // spend money it does not have.
    (void)event;
}

}  // namespace quant
