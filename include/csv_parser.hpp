#pragma once

#include <iosfwd>
#include <string>
#include <vector>

#include "event.hpp"

namespace quant {

/// Reads OHLCV bars out of a CSV file.
///
/// The std::ifstream exists only as a local inside parse_file(), so the file is
/// closed by RAII however the function exits -- including when it throws on a
/// malformed row. There is no close() to forget to call.
class CsvParser {
public:
    struct Options {
        char delimiter{','};
        bool has_header{true};
    };

    /// Comma-delimited, one header row.
    ///
    /// Two constructors rather than one with a default argument: a nested
    /// class's default member initializers may not be used before the end of
    /// the enclosing class, so `Options options = Options{}` cannot compile.
    CsvParser();
    explicit CsvParser(Options options);

    /// Parse `path` into bars, oldest first.
    /// Throws std::runtime_error if the file cannot be opened.
    std::vector<Bar> parse_file(const std::string& path) const;

    /// Parse an already-open stream. Separate from parse_file() so callers can
    /// parse a std::istringstream in a test without touching the filesystem.
    std::vector<Bar> parse_stream(std::istream& in) const;

    /// Split one CSV line on `delimiter`.
    /// Static because it needs no object state -- a pure utility on the class.
    static std::vector<std::string> split(const std::string& line,
                                          char delimiter);

private:
    Options options_;
};

}  // namespace quant
