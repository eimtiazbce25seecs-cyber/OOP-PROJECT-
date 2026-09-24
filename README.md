# QuantSimulator

A backtesting simulator for daily OHLCV price data, written in C++17 as an
Object-Oriented Programming course project.

It loads a CSV of daily bars, runs a trading strategy over them, tracks cash and
positions through a portfolio, and reports what the result would have been —
total return, Sharpe ratio, max drawdown, win rate — with an ImGui desktop
window in front and a plain ASCII report behind a flag.

---

## Architecture

The project is layered, and each layer only depends on the ones below it. That
is what keeps the engine testable without a window and the window replaceable
without touching the engine.

| Layer | Purpose | Built with |
|---|---|---|
| **GUI** | Window, panels, charts, live parameter editing | GLFW + OpenGL 3.3 + Dear ImGui + ImPlot |
| **Data** | Read and validate OHLCV CSV files; fetch live prices | [lazycsv](https://github.com/ashtum/lazycsv) (header-only), [cpr](https://github.com/libcpr/cpr) + [nlohmann/json](https://github.com/nlohmann/json) |
| **Math & Monte Carlo** | Statistics, performance metrics, bootstrap simulation | Standard library — `<random>`, `<cmath>`, `<thread>` |

### Design notes

- **The GUI is a separate CMake target** (`quant_gui`) from the engine
  (`quant`). The engine therefore has no GLFW or ImGui dependency at all, and
  `--nogui` links and runs without a display.
- **The engine is polymorphic at one point only** — `Strategy`. Concrete
  strategies are chosen at runtime by `quant::make_strategy(name)`, which
  returns a `std::unique_ptr<Strategy>`. `Backtest` never names a concrete
  strategy type.
- **Ownership is RAII throughout.** `CsvParser` owns its stream, `GuiApp` owns
  the GLFW window and both ImGui contexts and tears them down in reverse order,
  and `Portfolio` owns its positions. There is no manual `cleanup()` to forget.
- **The binary links the C++ runtime statically** (`-static`), so `main.exe`
  runs from Explorer or a plain `cmd` prompt without `libstdc++-6.dll` and
  `libgcc_s_seh-1.dll` needing to be on `PATH`.

---

## Repository layout

```
.
├── CMakeLists.txt          build definition; fetches all dependencies
├── data/
│   └── AAPL.csv            sample daily bars
├── include/                public headers — the engine's interface
│   ├── csv_parser.hpp      CsvParser: RAII file/stream parsing
│   ├── data_fetcher.hpp    fetch_twelvedata_prices: live prices over HTTP
│   ├── event.hpp           Event types passed between engine and portfolio
│   ├── metrics.hpp         PerformanceReport and the metric functions
│   ├── monte_carlo.hpp     MonteCarloSimulator and MonteCarloResult
│   ├── portfolio.hpp       Portfolio: cash, positions, equity curve
│   └── strategy.hpp        Strategy base class and the factory declaration
└── src/
    ├── main.cpp            entry point; GUI by default, --nogui for console
    ├── data/
    │   ├── csv_parser.cpp
    │   └── data_fetcher.cpp
    ├── engine/
    │   ├── backtest.hpp    Backtest: the event loop driver
    │   ├── backtest.cpp
    │   ├── event.cpp
    │   └── portfolio.cpp
    ├── math/
    │   ├── statistics.hpp  private: mean, stdev, covariance
    │   ├── statistics.cpp
    │   ├── metrics.cpp
    │   └── monte_carlo.cpp
    ├── strategies/
    │   ├── buy_and_hold.cpp
    │   ├── sma_crossover.cpp
    │   ├── mean_reversion.cpp
    │   └── factory.cpp     make_strategy() name → object
    └── ui/
        ├── gui.hpp         GuiApp: owns window + ImGui + ImPlot contexts
        ├── gui.cpp
        ├── report.hpp      the ASCII front end
        └── report.cpp
```

---

## Requirements

- A C++17 compiler, CMake ≥ 3.20, Ninja, and Git (CMake uses Git to download
  the dependencies).
- An OpenGL 3.3 capable GPU and driver — needed for the window only. The
  `--nogui` path runs anywhere.

### Windows: MSYS2 UCRT64 (the supported setup)

Install [MSYS2](https://www.msys2.org/), open the **UCRT64** shell, and run:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc \
                   mingw-w64-ucrt-x86_64-cmake \
                   mingw-w64-ucrt-x86_64-ninja
```

Git is already part of the MSYS2 base install.

> Use the **UCRT64** shell, not MSYS or MINGW64. Mixing toolchains produces
> link errors that are hard to read.

### Linux

```bash
sudo apt install build-essential cmake ninja-build git \
                 libgl1-mesa-dev xorg-dev
```

GLFW is built from source here, so the X11 development headers (`xorg-dev`) are
required.

### macOS

```bash
xcode-select --install
brew install cmake ninja
```

---

## Build

From the repository root:

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

The first configure downloads GLFW, Dear ImGui, ImPlot, lazycsv, cpr and
nlohmann/json into `build/_deps/`, so it needs a network connection. The
tag-pinned dependencies are fetched shallow (`GIT_SHALLOW`), so this pulls a few
megabytes rather than tens — normally well under a minute, though a slow link
takes longer and prints nothing while GLFW is downloading. cpr is the slow one:
it builds libcurl from source on the first configure, which is a couple of
minutes on its own. It is a **one-time cost**: afterwards `build/_deps/` is
reused and builds are fast.

If a download fails part-way through — a dropped connection, or DNS hiccuping —
**just run the configure command again.** Dependencies that already succeeded are
kept, so only the failed one is retried; you do not need to start over or delete
`build/`.

The executable is written to `build/main.exe` (or `build/main` on Linux/macOS).

To start over completely:

```bash
cmake -S . -B build -G Ninja --fresh
```

---

## Run

```bash
./build/main.exe                 # opens the GUI window
./build/main.exe --nogui         # ASCII report in the terminal
./build/main.exe --nogui sma     # pick a strategy
./build/main.exe --help
```

| Argument | Meaning |
|---|---|
| `--nogui` | Skip the window, print the console report |
| *strategy* | `buy_and_hold`, `sma`, or `mean_reversion` (default `sma`) |
| *csv-path* | OHLCV file to load (default `data/AAPL.csv`) |

Flags may appear in any position, so `main sma --nogui` and `main --nogui sma`
are equivalent.

**The GUI falls back to the console automatically** if a window cannot be
created — no display, no OpenGL 3.3, or a driver problem. It prints the reason
to `stderr` and runs the ASCII report instead of failing.

> **Until the `TODO` bodies are implemented**, `--nogui` stops at the strategy
> factory:
>
> ```
> [scaffold] function bodies are TODO -- grep for TODO under src/
>
> loaded 0 bars from data/AAPL.csv
> error: make_strategy: not implemented yet
> ```
>
> It exits with status 1. That is the scaffold telling you which piece is
> missing, not a build problem — see [Project status](#project-status). The GUI
> path does *not* do this: it opens normally, because the button that would call
> the engine is itself a `TODO`.

### VS Code

`.vscode/launch.json` is configured to debug `build/main.exe` under gdb, so
<kbd>F5</kbd> works after a build with no further setup. Add `--nogui` to the
`args` array there to debug the console path.

---

## Dependencies

All six are downloaded and built automatically by CMake's `FetchContent` — no
manual install step, and nothing is vendored into the repository. Versions are
pinned, so every clone builds the same code:

| Dependency | Version | Role |
|---|---|---|
| [GLFW](https://github.com/glfw/glfw) | `3.4` | Window, OpenGL context, input |
| [Dear ImGui](https://github.com/ocornut/imgui) | `v1.92.9b` | Immediate-mode GUI |
| [ImPlot](https://github.com/epezent/implot) | `v1.0` | Charts inside ImGui |
| [lazycsv](https://github.com/ashtum/lazycsv) | commit `6c36a9d` | CSV parsing |
| [cpr](https://github.com/libcpr/cpr) | `1.14.2` | HTTP client for live prices |
| [nlohmann/json](https://github.com/nlohmann/json) | `v3.12.0` | JSON parsing for the price response |

Four things worth knowing:

- **No GLEW or GLAD.** ImGui's OpenGL3 backend carries its own OpenGL function
  loader, so the stack is GLFW + `OpenGL::GL` and nothing else.
- **Dear ImGui and ImPlot ship no `CMakeLists.txt` upstream.** `FetchContent`
  only downloads their sources; the `imgui` and `implot` targets are declared
  in `CMakeLists.txt` and compiled from those sources directly.
- **Clones are shallow.** `GIT_SHALLOW TRUE` turns each into
  `git clone --depth 1 --branch <tag>`, which is a few megabytes instead of
  tens and makes the first configure far more likely to complete on a poor
  connection. lazycsv is the exception: it pins a commit hash rather than a tag,
  and `--depth 1 --branch` has nothing to land on for a bare commit.
- **cpr builds libcurl from source.** It is the largest dependency here by a
  wide margin, so the first configure takes noticeably longer than it used to.
  It uses Schannel for TLS on Windows, which is why there is still no OpenSSL to
  install and no `.dll` alongside the executable.

---

## Live prices

`fetch_twelvedata_prices()` fetches the latest price for a set of symbols from
[Twelve Data](https://twelvedata.com), so a simulation can be seeded from today
rather than from the last row of a CSV. It needs a free API key, which the
caller passes in:

```cpp
#include "data_fetcher.hpp"

const std::string api_key = "...";
auto prices = fetch_twelvedata_prices({"AAPL", "MSFT"}, api_key);
```

It returns a ticker → price `unordered_map` that is **empty** when nothing could
be fetched, and omits any symbol the API did not answer for. Check for the
ticker you want rather than assuming the map is complete:

```cpp
auto it = prices.find("AAPL");
if (it != prices.end()) {
    double s0 = it->second;
}
```

Nothing is thrown: a missing key, a rejected key, a timeout and a non-JSON
response each print one line to `stderr` and return an empty map, so a network
problem cannot take down a run that would otherwise work from CSV data.

> **The key is a secret, and the repository is public.** A literal in `main.cpp`
> is committed and pushed with everything else. An environment variable —
> `std::getenv("TWELVEDATA_API_KEY")` — keeps it out of the history, and a key
> that has already been committed should be rotated, because deleting the line
> does not remove it from the commits that already have it.

---

## Project status

This repository is a **teaching scaffold**. The architecture, the interfaces,
the build system and the GUI are complete and working; the algorithmic bodies
are the assignment.

**Complete:**

- The whole GUI shell — window, layout, panels, controls, status bar, and RAII
  teardown of every context.
- The build system, dependency fetching, and the static-link setup.
- The console front end.
- Every header in `include/` and `src/`, fully documented.

**Left to implement** — 62 `TODO` markers, found with:

```bash
grep -rn TODO src include
```

| File | What it needs |
|---|---|
| `src/data/csv_parser.cpp` | Parse the CSV into `Bar`s; validate and report bad rows |
| `src/engine/backtest.cpp` | The event loop: bars in, signals out, fills applied |
| `src/engine/portfolio.cpp` | Cash and position accounting, equity marking |
| `src/engine/event.cpp` | Event construction and dispatch |
| `src/math/statistics.cpp` | Mean, standard deviation, covariance |
| `src/math/metrics.cpp` | Returns, Sharpe, drawdown, win rate |
| `src/math/monte_carlo.cpp` | Bootstrap resampling across worker threads |
| `src/strategies/*.cpp` | The three strategies and the factory registry |
| `src/ui/report.cpp` | The ASCII report formatting |
| `src/ui/gui.cpp` | One call site: the "Run backtest" button |

**Consequence to expect:** the window opens, the controls respond, and the plot
axes render — but the equity curve stays empty until the stubs above are
implemented. A strategy stub returns no signal, so no trade is ever made. On the
console path, `--nogui` exits 1 once it reaches `make_strategy()`.

Both are correct behaviour for a scaffold, not bugs. The window is finished; the
arithmetic behind it is the exercise.

---

## Adding a strategy

1. Create `src/strategies/my_strategy.cpp`.
2. Define a class deriving from `quant::Strategy` and implement the two pure
   virtuals: `name()` and `on_bar(const Bar&)`, which returns
   `std::optional<Signal>`. Override `reset()` too if the strategy carries state
   that must not leak between runs.
3. Register the name in `quant::make_strategy()` in `src/strategies/factory.cpp`.
4. Add the new `.cpp` to the `quant` target in `CMakeLists.txt`.
5. Add the name to the combo box list in `GuiApp::draw_controls_panel()`.

---

## Pushing to GitHub

Check what remote is already configured:

```bash
git remote -v
```

**If a remote already exists**, commit and push:

```bash
git add .
git commit -m "Add GUI layer, dependency fetching and README"
git push
```

**If not**, create an empty repository on GitHub (no README, no `.gitignore` —
this project already has both), then:

```bash
git remote add origin https://github.com/<your-username>/<repo-name>.git
git branch -M main
git add .
git commit -m "Add GUI layer, dependency fetching and README"
git push -u origin main
```

### What is and is not committed

`build/` is **not** tracked — it holds generated object files, the executable,
and the downloaded dependencies, all of which are rebuilt by the commands above.
`.gitignore` covers it along with the usual CMake and compiler output. If it was
tracked previously, untrack it while keeping it on disk:

```bash
git rm -r --cached build
```

---

## Troubleshooting

**`CMake Error: could not find git for clone of ...`**
Install Git, or make sure it is on `PATH`. `FetchContent` shells out to Git to
download the dependencies.

**Configure dies part-way through a download**
Symptoms are `fatal: unable to access '...': Could not resolve host: github.com`
or `error: RPC failed; curl 56 Recv failure: Connection was reset`. These are
transient network failures, not a broken configuration — **re-run the configure
command**. Dependencies that already downloaded are kept, so each retry resumes
where the last one stopped rather than starting over.

**Configure fails on a machine behind a firewall**
`FetchContent` needs to reach GitHub at configure time. On a locked-down network,
the alternative is to clone the four dependencies manually and add them under
`external/` as committed source instead.

**`main.exe` will not start — missing `libstdc++-6.dll`**
The `-static` link flag did not reach the final link. It is set as
`target_link_options(quant PUBLIC -static)`; confirm it is still `PUBLIC` so the
executable inherits it.

**The window opens but the chart is empty**
Expected — see [Project status](#project-status). The stubs are unimplemented.

**`GLFW error 65542: WGL: The driver does not appear to support OpenGL`**
Update the graphics driver, or run with `--nogui`.
