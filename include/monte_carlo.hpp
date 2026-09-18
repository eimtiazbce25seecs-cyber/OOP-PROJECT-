#pragma once

#include <cstddef>
#include <random>
#include <vector>

namespace quant {

/// Where a Monte Carlo run ended up.
struct MonteCarloResult {
    double mean{0.0};
    double median{0.0};
    double p05{0.0};   ///< 5th percentile -- the pessimistic scenario
    double p95{0.0};   ///< 95th percentile -- the optimistic one
    /// Deciles 0, 10, ... 100 of the final values, for the UI to plot.
    std::vector<double> percentile_points;
};

/// Simulates future price paths to put a confidence band around a result.
///
/// The generator is a member rather than a local variable, which gives the
/// object identity: reseed() makes a whole run reproducible, something a
/// collection of free functions could not offer. Note also that simulate() and
/// bootstrap() are deliberately not const -- they advance rng_.
class MonteCarloSimulator {
public:
    explicit MonteCarloSimulator(unsigned seed = std::random_device{}());

    /// Geometric Brownian motion: dS = mu * S * dt + sigma * S * dW.
    /// `mu` and `sigma` are annualised; `horizon` is a number of trading days.
    MonteCarloResult simulate(std::size_t paths, std::size_t horizon,
                              double mu, double sigma, double start_price);

    /// Resample the historical returns with replacement instead of assuming
    /// they are normal. Keeps fat tails and skew that the GBM model erases.
    MonteCarloResult bootstrap(const std::vector<double>& historical_returns,
                               std::size_t paths, std::size_t horizon,
                               double start_price);

    /// Restart the generator from `seed`, so the next run repeats exactly.
    void reseed(unsigned seed);

private:
    /// Private, so nothing outside can hand out a generator whose state
    /// diverges from what this object thinks it is.
    std::mt19937_64 rng_;
};

}  // namespace quant
