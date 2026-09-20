cmake_minimum_required(VERSION 3.20)

# C as well as CXX: GLFW is a C library and needs the C language enabled before
# its own project() call runs.
project(QuantSimulator LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to a Debug build so F5 has symbols and the debugger is actually useful.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Emit build/compile_commands.json, which .vscode/settings.json points
# C_Cpp.default.compileCommands at. Set here rather than passed as a -D flag so
# it survives `--fresh` and a fresh clone: without the file IntelliSense has no
# include paths, and every #include in main.cpp is marked "file not found" --
# which reads exactly like a broken build when the build is in fact fine.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Everything we fetch is built static. Set before the FetchContent calls so the
# sub-projects inherit it rather than each defaulting independently.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

include(FetchContent)

# ---------------------------------------------------------------------------
# GLFW -- built from source rather than taken from MSYS2
#
# MSYS2 packages GLFW, but installing it would mean modifying the system, and
# its shared build would put libglfw3.dll on the dependency list -- which is the
# exact class of problem the -static flag below exists to prevent. Building from
# source keeps the executable self-contained and needs no pacman step on a
# teammate's machine. GLFW has no external dependencies on Windows.
# ---------------------------------------------------------------------------
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
)

# ---------------------------------------------------------------------------
# Dear ImGui and ImPlot -- neither is packaged by MSYS2, so both are fetched.
#
# Both ship no CMakeLists.txt upstream, so MakeAvailable only unpacks the source
# tree; the targets below are ours to declare. Note that the single-argument
# FetchContent_Populate() was removed in CMake 4.0 and this project is on 4.3.3,
# so MakeAvailable is not merely preferred here, it is the only option.
#
# Tags verified against the remotes: ImPlot v1.0 selects on IMGUI_VERSION_NUM
# and has an explicit branch for >= 1.91.72 that uses the reworked 1.92 font API
# (ImGuiWindow::FontRefSize), so it is compatible with ImGui 1.92.9b.
#
# GIT_SHALLOW on all three: they are large repositories whose history we have no
# use for, and a full clone of each is tens of megabytes. With a tag GIT_TAG,
# CMake turns this into `git clone --depth 1 --branch <tag>`, which is a couple
# of megabytes and, on a slow or flaky link, the difference between configuring
# and failing part-way through.
# ---------------------------------------------------------------------------
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.92.9b
    GIT_SHALLOW    TRUE
)
FetchContent_Declare(implot
    GIT_REPOSITORY https://github.com/epezent/implot.git
    GIT_TAG        v1.0
    GIT_SHALLOW    TRUE
)
# lazycsv has no release tags at all, so this pins a commit on master. It does
# ship a CMakeLists.txt, but that only adds its test subdirectory when
# BUILD_TESTING is set, which it is not here.
#
# Deliberately NOT shallow: GIT_SHALLOW only works when GIT_TAG names a branch
# or tag, since it becomes `--depth 1 --branch`. Against a bare commit hash the
# checkout has nothing to land on. The whole repository is under a megabyte, so
# there is nothing to gain here anyway.
FetchContent_Declare(lazycsv
    GIT_REPOSITORY https://github.com/ashtum/lazycsv.git
    GIT_TAG        6c36a9d66be1e652b0c06d4e34fb053b93caf0ff
)

FetchContent_MakeAvailable(glfw imgui implot lazycsv)

find_package(OpenGL REQUIRED)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
# SYSTEM so our -Wall -Wextra -Wpedantic does not fire on third-party headers.
# No GLEW: the OpenGL3 backend carries its own function loader.
target_include_directories(imgui SYSTEM PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

add_library(implot STATIC
    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_items.cpp
)
target_include_directories(implot SYSTEM PUBLIC ${implot_SOURCE_DIR})
target_link_libraries(implot PUBLIC imgui)

# ---------------------------------------------------------------------------
# quant -- the engine, data and math layers. No GUI dependency: the console
# path in main.cpp links this on its own, and the window is a separate target.
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

# lazycsv is an INTERFACE target, so this only adds its include directory.
target_link_libraries(quant PRIVATE lazycsv)

target_compile_options(quant PRIVATE -Wall -Wextra -Wpedantic)

# Link the C++ runtime statically. Without this the binary needs libstdc++-6.dll
# and libgcc_s_seh-1.dll from C:\msys64\ucrt64\bin on PATH, so it only runs from
# a shell that has that on PATH -- not from Explorer, and not from the MSYS
# shell. PUBLIC so the executable inherits the flag.
target_link_options(quant PUBLIC -static)

# ---------------------------------------------------------------------------
# quant_gui -- the GUI layer, kept separate so the core stays free of GLFW.
# ---------------------------------------------------------------------------
add_library(quant_gui STATIC src/ui/gui.cpp)
target_include_directories(quant_gui PUBLIC src)
target_link_libraries(quant_gui PUBLIC quant PRIVATE imgui implot glfw)
target_compile_options(quant_gui PRIVATE -Wall -Wextra -Wpedantic)

# ---------------------------------------------------------------------------
# Executable
# ---------------------------------------------------------------------------
add_executable(quant_sim src/main.cpp)
target_link_libraries(quant_sim PRIVATE quant quant_gui)

# main.cpp reaches into src/ for the private headers ("engine/backtest.hpp").
target_include_directories(quant_sim PRIVATE src)

# Output stays named main.exe on purpose: .vscode/launch.json debugs
# ${workspaceFolder}/build/main.exe, so F5 keeps working untouched.
set_target_properties(quant_sim PROPERTIES OUTPUT_NAME main)
