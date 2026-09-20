// The GUI shell: one GLFW window, one ImGui context, one ImPlot context.
//
// This file is infrastructure, so it is written complete. The analytics it
// displays are not -- every place that would call into the engine is marked
// TODO, so the window builds, opens and responds before any of the project's
// function bodies are written.

#include "gui.hpp"

#include <cstdio>
#include <stdexcept>
#include <utility>

#include <GLFW/glfw3.h>  // pulls in <GL/gl.h> too, which is where glClear lives

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"

namespace quant::ui {
namespace {

// GL 3.3 core has no deprecated functionality, and #version 150 is the GLSL
// that ships with it. ImGui's OpenGL3 backend targets exactly this pair.
constexpr const char* kGlslVersion = "#version 150";

// The widget state strings live in fixed buffers for InputText, which is a
// C API underneath and wants a char* it can write into.
constexpr std::size_t kPathBufferSize = 512;

constexpr double kCommissionMin = 0.0;
constexpr double kCommissionMax = 0.05;

constexpr float kControlsPanelWidth = 340.0f;
constexpr float kEquityChartHeight = 260.0f;

// GLFW reports failures through this callback rather than by returning an
// error, so without it a driver problem is a silent black window.
void glfw_error_callback(int error, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

}  // namespace

// ---------------------------------------------------------------------------
// Lifetime
// ---------------------------------------------------------------------------

GuiApp::GuiApp(GuiConfig config)
    // config_ is initialised before data_path_ because it is declared first --
    // member init order follows declaration order, not this list. Reading the
    // parameter here instead would read the object std::move already emptied.
    : config_(std::move(config)), data_path_(config_.data_path) {
    glfwSetErrorCallback(glfw_error_callback);

    if (glfwInit() != GLFW_TRUE) {
        throw std::runtime_error(
            "glfwInit failed -- no display, or no OpenGL support available");
    }

    // Core profile 3.3: the oldest version every desktop driver supports, and
    // the one ImGui's OpenGL3 backend is written against.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window_ = glfwCreateWindow(config_.width, config_.height,
                               config_.title.c_str(), nullptr, nullptr);
    if (window_ == nullptr) {
        shutdown();
        throw std::runtime_error(
            "could not create an OpenGL 3.3 window -- update your graphics driver");
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);  // vsync: the loop would otherwise spin a core flat

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    imgui_context_ = true;

    ImPlot::CreateContext();
    implot_context_ = true;

    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // The layout is one fixed full-viewport window, so there is no position or
    // size worth remembering -- and no imgui.ini left in the repo root.
    io.IniFilename = nullptr;

    if (!ImGui_ImplGlfw_InitForOpenGL(window_, true)) {
        shutdown();
        throw std::runtime_error("the ImGui GLFW backend failed to initialise");
    }
    imgui_glfw_ = true;

    if (!ImGui_ImplOpenGL3_Init(kGlslVersion)) {
        shutdown();
        throw std::runtime_error("the ImGui OpenGL3 backend failed to initialise");
    }
    imgui_opengl_ = true;
}

GuiApp::~GuiApp() {
    shutdown();
}

void GuiApp::shutdown() noexcept {
    // Strict reverse of construction order: the backends refer to the ImGui
    // context, and ImPlot refers to it too, so both must go before it does.
    if (imgui_opengl_) {
        ImGui_ImplOpenGL3_Shutdown();
        imgui_opengl_ = false;
    }
    if (imgui_glfw_) {
        ImGui_ImplGlfw_Shutdown();
        imgui_glfw_ = false;
    }
    if (implot_context_) {
        ImPlot::DestroyContext();
        implot_context_ = false;
    }
    if (imgui_context_) {
        ImGui::DestroyContext();
        imgui_context_ = false;
    }
    if (window_ != nullptr) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    // Only safe to call after a successful glfwInit, which is the only way we
    // reach a constructed window_. The constructor's failure path runs this
    // too, where it is a no-op for everything not yet created.
    glfwTerminate();
}

// ---------------------------------------------------------------------------
// Frame loop
// ---------------------------------------------------------------------------

int GuiApp::run() {
    while (glfwWindowShouldClose(window_) == GLFW_FALSE) {
        glfwPollEvents();

        // A minimised window has a zero-sized framebuffer. Rendering into it
        // is wasted work at best, so wait for the user instead of spinning.
        if (glfwGetWindowAttrib(window_, GLFW_ICONIFIED) != 0) {
            glfwWaitEventsTimeout(0.1);
            continue;
        }

        draw_frame();

        int fb_width = 0;
        int fb_height = 0;
        glfwGetFramebufferSize(window_, &fb_width, &fb_height);
        glViewport(0, 0, fb_width, fb_height);
        glClearColor(0.06f, 0.06f, 0.07f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window_);
    }
    return 0;
}

void GuiApp::draw_frame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // One full-viewport host window rather than free-floating panels: it gives
    // the layout a single owner, so the menu bar, the panels and the status
    // bar cannot drift apart as the window is resized.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    constexpr ImGuiWindowFlags kHostFlags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 4.0f));
    ImGui::Begin("##host", nullptr, kHostFlags);
    ImGui::PopStyleVar(3);

    draw_menu_bar();

    // Reserve the status bar's row first, so the panels above it can take
    // whatever height is left without pushing it off the bottom.
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const float body_height = available.y - ImGui::GetFrameHeightWithSpacing();

    ImGui::BeginChild("##controls", ImVec2(kControlsPanelWidth, body_height),
                      ImGuiChildFlags_Borders);
    draw_controls_panel();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##results", ImVec2(0.0f, body_height), ImGuiChildFlags_Borders);
    draw_plot_panel();
    draw_report_panel();
    ImGui::EndChild();

    draw_status_bar();

    ImGui::End();

    // Finalises the frame and builds the draw lists. This has to happen before
    // the caller asks for them: GetDrawData() returns null until Render() runs,
    // and the OpenGL3 backend dereferences that pointer without checking.
    ImGui::Render();
}

void GuiApp::draw_menu_bar() {
    if (!ImGui::BeginMenuBar()) {
        return;
    }

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Quit", "Alt+F4")) {
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help")) {
        // Disabled items are a honest way to show the pinned versions on
        // screen: this is what the build actually fetched, not a guess.
        ImGui::MenuItem("Dear ImGui " IMGUI_VERSION, nullptr, false, false);
        ImGui::MenuItem("ImPlot " IMPLOT_VERSION, nullptr, false, false);
        ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
}

// ---------------------------------------------------------------------------
// Panels
// ---------------------------------------------------------------------------

void GuiApp::draw_controls_panel() {
    ImGui::SeparatorText("Data");

    char path_buffer[kPathBufferSize];
    std::snprintf(path_buffer, sizeof(path_buffer), "%s", data_path_.c_str());
    if (ImGui::InputText("CSV path", path_buffer, sizeof(path_buffer))) {
        data_path_ = path_buffer;
    }
    ImGui::TextDisabled("Relative to the working directory,\nwhich is the repo root.");

    ImGui::SeparatorText("Strategy");

    // TODO(student): drive this from the factory's registry once
    // quant::make_strategy() is implemented, so the combo box and the engine
    // cannot disagree about which strategies exist.
    const char* const kStrategies[] = {"Buy and hold", "SMA crossover", "Mean reversion"};
    ImGui::Combo("Model", &strategy_index_, kStrategies, IM_ARRAYSIZE(kStrategies));

    ImGui::SeparatorText("Execution");

    ImGui::InputDouble("Initial cash", &initial_cash_, 1000.0, 10000.0, "%.2f");
    ImGui::SliderInt("Order size", &order_size_, 1, 10000);
    ImGui::SliderScalar("Commission", ImGuiDataType_Double, &commission_,
                        &kCommissionMin, &kCommissionMax, "%.4f");

    ImGui::SeparatorText("Run");

    ImGui::BeginDisabled(running_);
    if (ImGui::Button("Run backtest", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f))) {
        // TODO(student): this is the seam between the GUI and the engine. It
        // should, in order --
        //   1. parse the CSV at data_path_ with CsvParser::parse_file(),
        //   2. build a BacktestConfig from data_path_, initial_cash_,
        //      commission_ and order_size_,
        //   3. make a strategy with quant::make_strategy(name),
        //   4. run Backtest::run(bars),
        //   5. copy Backtest::equity_curve() into equity_curve_ and the
        //      returned PerformanceReport into the report table below.
        // The widget state above is already wired, so none of that touches
        // this function's signature.
        status_ = "run is not wired up yet -- see the TODO in draw_controls_panel()";
    }
    ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::TextDisabled(
        "The controls are live and the layout is final.\n"
        "Only the engine call behind this button is left\n"
        "to implement.");
}

void GuiApp::draw_plot_panel() {
    ImGui::SeparatorText("Equity curve");

    // BeginPlot wants a size; pass a fixed height so the report table below
    // keeps its place instead of being pushed off the panel.
    if (ImPlot::BeginPlot("##equity", ImVec2(-1.0f, kEquityChartHeight))) {
        ImPlot::SetupAxes("bar", "equity");

        // TODO(student): the draw call below is already correct -- it needs no
        // change once equity_curve_ is filled in. One point is not a line, so
        // the guard is for the empty and single-bar cases, not a placeholder.
        if (equity_curve_.size() >= 2) {
            ImPlot::PlotLine("equity", equity_curve_.data(),
                             static_cast<int>(equity_curve_.size()));
        }

        ImPlot::EndPlot();
    }

    if (equity_curve_.empty()) {
        ImGui::TextDisabled(
            "The axes above are real; the line appears once the strategy,\n"
            "Portfolio and Backtest bodies are implemented.");
    }
}

void GuiApp::draw_report_panel() {
    ImGui::SeparatorText("Performance report");

    constexpr ImGuiTableFlags kTableFlags =
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;

    if (ImGui::BeginTable("##report", 2, kTableFlags)) {
        ImGui::TableSetupColumn("metric");
        ImGui::TableSetupColumn("value");
        ImGui::TableHeadersRow();

        // TODO(student): replace each placeholder with the matching field of
        // the metrics::PerformanceReport that Backtest::run() returns. The
        // rows are already laid out; only the second column changes.
        const char* const kMetrics[] = {
            "Total return", "Annualised return", "Annualised volatility",
            "Sharpe ratio",   "Max drawdown",      "Win rate",
            "Trades",
        };

        for (const char* metric : kMetrics) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(metric);
            ImGui::TableNextColumn();
            ImGui::TextDisabled("--");
        }

        ImGui::EndTable();
    }
}

void GuiApp::draw_status_bar() {
    ImGui::Separator();
    ImGui::TextUnformatted(status_.c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("| OpenGL 3.3 | %.0f FPS", ImGui::GetIO().Framerate);
}

}  // namespace quant::ui
