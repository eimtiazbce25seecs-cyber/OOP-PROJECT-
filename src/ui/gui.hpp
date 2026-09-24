#pragma once

#include <string>

// Forward declaration rather than #include <GLFW/glfw3.h>: this header is
// included by main.cpp, and keeping GLFW out of it means the entry point does
// not drag in the whole windowing API just to name the class.
struct GLFWwindow;

namespace quant::ui {

/// Window and startup settings. Plain data, so it is copyable and can be
/// defaulted as a whole.
struct GuiConfig {
    std::string title{"QuantSimulator"};
    std::string data_path{"data/aapl_us_d.csv"};
    std::string strategy{"sma"};
    int width{1280};
    int height{720};
};

/// Owns the GLFW window, the ImGui context and the ImPlot context.
///
/// Non-copyable, and the destructor tears everything down in reverse order of
/// construction. That is the same RAII argument CsvParser makes about
/// std::ifstream, one level up: there is no cleanup() to forget to call, and
/// the contexts are released however run() exits -- normally, or by exception.
///
/// This is the GUI *shell*. It opens a window, lays out the panels and holds
/// the widget state, but deliberately contains no analytics: the panels call
/// into the engine only at the points marked TODO, so the window builds and
/// runs before any of the project's function bodies are written.
class GuiApp {
public:
    /// Throws std::runtime_error if GLFW, the window or the GL contexts cannot
    /// be created.
    explicit GuiApp(const GuiConfig& config);

    ~GuiApp();

    GuiApp(const GuiApp&) = delete;
    GuiApp& operator=(const GuiApp&) = delete;

    /// Runs the frame loop until the window is closed.
    /// Returns 0 on a normal exit.
    int run();

private:
    void draw_frame();
    void draw_menu_bar();
    void draw_controls_panel();
    void draw_plot_panel();
    void draw_status_bar();

    /// Releases everything that was created, in reverse order. Idempotent,
    /// because the constructor calls it on its own failure paths -- where the
    /// destructor will never run, since the object never finished being built.
    void shutdown() noexcept;

    GuiConfig config_;
    GLFWwindow* window_{nullptr};

    // Contexts are tracked separately from window_ so the destructor can tear
    // down exactly what was created, even if construction threw part-way.
    bool imgui_context_{false};
    bool implot_context_{false};
    bool imgui_glfw_{false};
    bool imgui_opengl_{false};

    // ---- Widget state -----------------------------------------------------
    // Held here rather than in the widgets because ImGui is immediate mode:
    // the widgets are redrawn every frame and store nothing themselves.
    std::string data_path_;
    int strategy_index_{0};
    double initial_cash_{100'000.0};
    double commission_{0.001};
    int order_size_{100};
    std::string status_{"ready"};
    bool show_about_{false};

    // File > Quit sets this. Separate from glfwWindowShouldClose because that
    // only becomes true when the window manager closes the window, and the
    // menu has to be able to end the loop without one.
    bool quit_requested_{false};
};

}  // namespace quant::ui
