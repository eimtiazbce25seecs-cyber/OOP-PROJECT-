cmake_minimum_required(VERSION 3.20)

project(QuantSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to a Debug build so F5 has symbols and the debugger is actually useful.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# quant -- everything except main()
#
# Grouped into one library rather than a target per directory: the pieces are
# not independently useful, and a single target keeps the link line honest.
# Add new .cpp files here as the course goes on.
# ---------------------------------------------------------------------------
add_library(quant STATIC
    src/data/csv_parser.cpp
    src/engine/backtest.cpp
    src/engine/event.cpp
    src/engine/portfolio.cpp
    src/math/metrics.cpp
    src/math/monte_carlo.cpp
    src/math/statistics.cpp
    src/strategies/buy_and_hold.cpp
    src/strategies/factory.cpp
    src/strategies/mean_reversion.cpp
    src/strategies/sma_crossover.cpp
    src/ui/report.cpp
)

target_include_directories(quant
    PUBLIC  include      # public headers, propagated to anything linking quant
    PRIVATE src          # private headers: engine/, ui/, math/statistics.hpp
)

target_compile_options(quant PRIVATE -Wall -Wextra -Wpedantic)

# Link the C++ runtime statically. Without this the binary needs libstdc++-6.dll
# and libgcc_s_seh-1.dll from C:\msys64\ucrt64\bin on PATH, so it only runs from
# a shell that has that on PATH -- not from Explorer, and not from the MSYS
# shell. PUBLIC so the executable inherits the flag.
target_link_options(quant PUBLIC -static)

# ---------------------------------------------------------------------------
# Executable
# ---------------------------------------------------------------------------
add_executable(quant_sim src/main.cpp)
target_link_libraries(quant_sim PRIVATE quant)

# main.cpp reaches into src/ for the private headers ("engine/backtest.hpp").
target_include_directories(quant_sim PRIVATE src)

# Output stays named main.exe on purpose: .vscode/launch.json debugs
# ${workspaceFolder}/build/main.exe, so F5 keeps working untouched.
set_target_properties(quant_sim PROPERTIES OUTPUT_NAME main)
