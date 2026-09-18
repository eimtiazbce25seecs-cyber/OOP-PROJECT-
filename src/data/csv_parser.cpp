#include "csv_parser.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace quant {

CsvParser::CsvParser() = default;

CsvParser::CsvParser(Options options) : options_(std::move(options)) {}

std::vector<Bar> CsvParser::parse_file(const std::string& path) const {
    // TODO: open the file with std::ifstream, and if it did not open throw
    // std::runtime_error with the path in the message -- a silent empty vector
    // is the worst possible failure mode here. Then delegate to parse_stream.
    // RAII closes the file however this returns.
    (void)path;
    return {};
}

std::vector<Bar> CsvParser::parse_stream(std::istream& in) const {
    // TODO: read line by line.
    //   - skip the first line when options_.has_header is true
    //   - split on options_.delimiter
    //   - convert the fields into a Bar, oldest first
    //   - decide what a malformed row should do: skip it, or throw? Whatever
    //     you pick, the comment above parse_file in the header promises one of
    //     those two behaviours, so keep them consistent.
    (void)in;
    return {};
}

std::vector<std::string> CsvParser::split(const std::string& line, char delimiter) {
    // TODO: walk the string, cutting at each `delimiter`.
    // A trailing delimiter should still produce a final empty field.
    (void)line;
    (void)delimiter;
    return {};
}

}  // namespace quant
