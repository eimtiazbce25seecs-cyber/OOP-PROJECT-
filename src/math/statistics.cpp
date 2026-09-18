#include "statistics.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace quant::stats {

double mean(const std::vector<double>& values) {
    // TODO: sum / size. Return 0 for an empty vector rather than dividing by
    // zero -- callers here treat "no data" as "no signal".
    (void)values;
    return 0.0;
}

double stdev(const std::vector<double>& values) {
    // TODO: sample standard deviation -- divide by (n - 1), not n.
    // Return 0 when there are fewer than 2 values, since a single
    // observation has no spread to speak of.
    (void)values;
    return 0.0;
}

double percentile(std::vector<double> values, double p) {
    // TODO: sort, then interpolate between the two nearest ranks.
    // `p` is 0..100, so convert it to a fractional rank before indexing, and
    // be careful with the ends of the range.
    // Note the parameter is by value on purpose -- see the header.
    (void)p;
    (void)values;
    return 0.0;
}

double median(const std::vector<double>& values) {
    // TODO: percentile(values, 50.0).
    (void)values;
    return 0.0;
}

}  // namespace quant::stats
