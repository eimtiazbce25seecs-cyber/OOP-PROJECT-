#pragma once

#include <cstddef>
#include <vector>

namespace quant::stats {

/// Small numeric helpers shared by metrics.cpp and monte_carlo.cpp.
///
/// Free functions in a namespace rather than a class full of static methods:
/// there is no state here, so there is nothing to instantiate or protect.

/// Arithmetic mean. Returns 0 for an empty input.
double mean(const std::vector<double>& values);

/// Sample standard deviation (divides by n-1). Returns 0 if fewer than 2 values.
double stdev(const std::vector<double>& values);

/// Linear-interpolated percentile, `p` in [0, 100].
/// Takes the vector by value because it needs to sort, and the caller's data
/// should not be reordered as a side effect.
double percentile(std::vector<double> values, double p);

double median(const std::vector<double>& values);

}  // namespace quant::stats
