#include "data_fetcher.hpp"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <string>

namespace {

constexpr const char* kPriceEndpoint = "https://api.twelvedata.com/price";

/// Renders one field of a JSON object as text for an error message, without
/// ever throwing.
///
/// The obvious `object.value("message", std::string{})` is a trap here: value()
/// converts to the *default's* type, so a field the API sent as a number or an
/// object throws nlohmann::type_error rather than falling back -- and an
/// exception escaping this translation unit would break the promise that a
/// failed fetch reports on stderr and returns an empty map.
std::string field_as_text(const nlohmann::json& object, const char* key,
                          const char* fallback) {
    if (!object.is_object() || !object.contains(key)) {
        return fallback;
    }

    const nlohmann::json& node = object[key];
    if (node.is_string()) {
        return node.get<std::string>();
    }
    if (node.is_null()) {
        return fallback;
    }
    return node.dump();  // Numbers and anything unexpected, rendered as-is.
}

/// Reads the "price" field out of one node of a Twelve Data response.
///
/// The API sends the value as a JSON *string* ("142.51"), which is the shape
/// this reads; it also accepts a bare number, because the same field has been
/// observed as one and a silent 0.0 -- stod() on garbage, or an unchecked
/// get<double>() -- is far worse than a missing entry. Returns false when the
/// node holds no usable price, so the caller can leave that ticker out of the
/// result instead of recording a bogus zero.
bool read_price(const nlohmann::json& node, double& out) {
    if (!node.is_object() || !node.contains("price")) {
        return false;
    }

    const nlohmann::json& price = node["price"];
    try {
        out = price.is_string() ? std::stod(price.get<std::string>())
                                : price.get<double>();
        return true;
    } catch (const std::exception&) {
        return false;  // Present, but not something that parses as a number.
    }
}

/// True when the response is Twelve Data's error envelope rather than data.
///
/// Errors arrive as an object carrying "status":"error" with a numeric "code"
/// and a "message" -- for example a rejected API key, or a symbol the plan does
/// not cover. Without this check an error is indistinguishable from a batch
/// response for a symbol that simply was not recognised, and the real cause
/// (an expired key, say) never reaches the console.
bool is_error_response(const nlohmann::json& parsed) {
    if (!parsed.is_object() || parsed.contains("price")) {
        return false;
    }

    // is_string() first: comparing a non-string node against "error" would
    // itself throw, which is the failure this function exists to prevent.
    const bool status_is_error = parsed.contains("status") &&
                                 parsed["status"].is_string() &&
                                 parsed["status"].get<std::string>() == "error";
    return status_is_error ||
           (parsed.contains("code") && parsed.contains("message"));
}

}  // namespace

std::unordered_map<std::string, double> fetch_twelvedata_prices(
    const std::vector<std::string>& tickers,
    const std::string& api_key) {
    std::unordered_map<std::string, double> live_prices;

    // Drop blank entries up front. They would put a stray comma in the symbol
    // list, and the batch response has no key that could map a blank back to a
    // caller -- so there is nothing useful to do with them later.
    std::vector<std::string> wanted;
    wanted.reserve(tickers.size());
    for (const std::string& ticker : tickers) {
        if (!ticker.empty()) {
            wanted.push_back(ticker);
        }
    }

    if (wanted.empty()) {
        return live_prices;  // Nothing to ask for; no request worth making.
    }
    if (api_key.empty()) {
        std::cerr << "[DataFetcher] no API key supplied -- skipping live prices\n";
        return live_prices;
    }

    // Join into the comma-separated form the endpoint takes: "AAPL,MSFT,JNJ".
    std::string symbol_param;
    for (const std::string& ticker : wanted) {
        if (!symbol_param.empty()) {
            symbol_param += ',';
        }
        symbol_param += ticker;
    }

    // Symbols and key go through cpr::Parameters rather than into the URL by
    // hand, so cpr escapes them. Building the query string manually is how a
    // key or a symbol containing '&' or '+' ends up silently truncated.
    const cpr::Response response = cpr::Get(
        cpr::Url{kPriceEndpoint},
        cpr::Parameters{{"symbol", symbol_param}, {"apikey", api_key}},
        cpr::Timeout{5000});  // milliseconds

    // status_code is 0 when the request never completed. cpr keeps the reason
    // in error.message, which is the difference between "wrong key" and
    // "no DNS" -- worth printing rather than reporting a bare 0.
    if (response.status_code == 0) {
        std::cerr << "[DataFetcher] request failed: " << response.error.message
                  << '\n';
        return live_prices;
    }
    if (response.status_code != 200) {
        std::cerr << "[DataFetcher] HTTP " << response.status_code << '\n';
        return live_prices;
    }

    nlohmann::json parsed;
    try {
        parsed = nlohmann::json::parse(response.text);
    } catch (const std::exception& e) {
        // Not JSON at all -- a proxy or captive portal answering instead of the
        // API. Report the parse failure; there is nothing to salvage.
        std::cerr << "[DataFetcher] JSON parse error: " << e.what() << '\n';
        return live_prices;
    }

    if (is_error_response(parsed)) {
        // A rejected key and an unknown symbol both land here, and the message
        // is the only thing that tells them apart -- so print what the API
        // actually said rather than a generic failure.
        std::cerr << "[DataFetcher] Twelve Data error "
                  << field_as_text(parsed, "code", "?") << ": "
                  << field_as_text(parsed, "message", "no message") << '\n';
        return live_prices;
    }

    // A single symbol comes back as the price object on its own, with no symbol
    // key wrapping it: {"price": "142.51"}. Only treat it that way for a
    // one-symbol request -- with several symbols an unwrapped object would mean
    // something unexpected, and attributing it to wanted.front() would put one
    // asset's price on another's ticker.
    if (wanted.size() == 1 && parsed.is_object() && parsed.contains("price")) {
        double price = 0.0;
        if (read_price(parsed, price)) {
            live_prices[wanted.front()] = price;
        } else {
            std::cerr << "[DataFetcher] unreadable price for " << wanted.front()
                      << '\n';
        }
        return live_prices;
    }

    // Several symbols come back as one entry per symbol, keyed by ticker. A
    // symbol the API did not answer for is absent, or carries its own error
    // object, and is reported and skipped either way.
    if (parsed.is_object()) {
        for (const std::string& ticker : wanted) {
            double price = 0.0;
            if (parsed.contains(ticker) && read_price(parsed[ticker], price)) {
                live_prices[ticker] = price;
            } else {
                std::cerr << "[DataFetcher] no price for " << ticker << '\n';
            }
        }
    }

    return live_prices;
}
