#ifndef DATA_FETCHER_HPP
#define DATA_FETCHER_HPP

#include <string>
#include <unordered_map>
#include <vector>

/// Fetches the latest live price (S0) for a set of asset tickers from Twelve
/// Data, for seeding a simulation from today's prices rather than the last row
/// of a CSV.
///
/// Returns one ticker -> price entry per symbol the API answered for. The map
/// is empty when nothing could be fetched, and a symbol that failed is simply
/// absent from it, so callers must check for the ticker they need rather than
/// assuming a full map:
///
///     auto prices = fetch_twelvedata_prices({"AAPL"}, key);
///     auto it = prices.find("AAPL");
///     if (it != prices.end()) { double s0 = it->second; ... }
///
/// A failure is reported on stderr and never thrown -- a network problem should
/// not take down a backtest that could otherwise run on CSV data.
///
/// An empty api_key, or an empty ticker list, returns an empty map without
/// making a request.
std::unordered_map<std::string, double> fetch_twelvedata_prices(
    const std::vector<std::string>& tickers,
    const std::string& api_key);

#endif  // DATA_FETCHER_HPP
