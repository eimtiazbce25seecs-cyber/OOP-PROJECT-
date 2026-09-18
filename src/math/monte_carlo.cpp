#include "monte_carlo.hpp"

#include <cmath>
#include <cstddef>
#include <random>
#include <vector>

#include "statistics.hpp"

namespace quant {

MonteCarloSimulator::MonteCarloSimulator(unsigned seed) : rng_(seed) {}

void MonteCarloSimulator::reseed(unsigned seed) {
    // TODO: restart the generator from `seed`.
    (void)seed;
}

MonteCarloResult MonteCarloSimulator::simulate(std::size_t paths,
                                               std::size_t horizon,
                                               double mu, double sigma,
                                               double start_price) {
    // TODO: for each of `paths` paths, step the price `horizon` times:
    //     S *= exp((mu - 0.5 * sigma * sigma) * dt + sigma * sqrt(dt) * z)
    // with dt = 1.0 / 252 and z drawn from std::normal_distribution<double>(0, 1).
    // Note rng_ is a member, so draw from it directly rather than making a
    // local generator -- that is what makes reseed() meaningful.
    //
    // Collect only each path's FINAL value, then summarise those `paths`
    // values: mean, median, and the percentiles in MonteCarloResult. Build
    // percentile_points as the 0, 10, 20 ... 100 deciles.
    // quant::stats::percentile is in math/statistics.hpp.
    (void)paths;
    (void)horizon;
    (void)mu;
    (void)sigma;
    (void)start_price;
    return {};
}

MonteCarloResult MonteCarloSimulator::bootstrap(
    const std::vector<double>& historical_returns, std::size_t paths,
    std::size_t horizon, double start_price) {
    // TODO: same shape as simulate(), but instead of drawing a normal z each
    // step, pick a return at random from historical_returns (with
    // replacement) and compound it. Use a uniform distribution over the
    // indices, and remember to handle an empty input.
    //
    // Why bother: this keeps fat tails and skew that the normal model erases,
    // so the tails of the resulting band are usually wider -- and more
    // honest -- than simulate()'s.
    (void)historical_returns;
    (void)paths;
    (void)horizon;
    (void)start_price;
    return {};
}

}  // namespace quant
