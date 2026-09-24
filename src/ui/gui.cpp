// GuiApp: the GLFW + ImGui + ImPlot window.

#include "ui/gui.hpp"

// GLFW includes <GL/gl.h> itself (nothing here defines GLFW_INCLUDE_NONE), and
// that is what supplies glViewport / glClearColor / glClear below. All three
// are OpenGL 1.1 entry points, so the 1.1-only header mingw ships is enough;
// every modern function ImGui needs is resolved by the OpenGL3 backend's own
// loader, inside its own translation unit.
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>

#include <cfloat>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace quant::ui {
namespace {

// GLSL 1.50 is the version that pairs with a 3.3 *core* profile. The pairing
// matters: a core profile rejects anything below #version 140, so the "#version
// 130" the ImGui example uses with a default (compatibility) context would fail
// to compile here and leave a blank window with no error on screen.
constexpr const char* kGlslVersion = "#version 150";

// The strategy list is a plain array rather than a call into the engine,
// because the GUI has to build before make_strategy() exists. Once
// include/strategy.hpp declares a registry, read the names from that instead --
// otherwise this list can drift out of step with the strategies that can
// actually be constructed, and the combo box will offer a name that throws.
const char* const kStrategyNames[] = {"buy_and_hold", "sma", "mean_reversion"};
constexpr int kStrategyCount =
    static_cast<int>(sizeof(kStrategyNames) / sizeof(kStrategyNames[0]));

// GLFW reports every failure through this callback and nowhere else: glfwInit
// returns only GLFW_FALSE, and glfwCreateWindow returns only nullptr. Without
// it a failure is a bare "could not create window" with no driver message.
void glfw_error_callback(int error, const char* description) {
    std::fprintf(stderr, "glfw error %d: %s\n", error, description);
}

}  // namespace

GuiApp::GuiApp(const GuiConfig& config)
    : config_(config), data_path_(config.data_path) {
    glfwSetErrorCallback(glfw_error_callback);

    if (glfwInit() != GLFW_TRUE) {
        // Nothing was created, so there is nothing to unwind: the destructor
        // will never run for an object whose constructor threw, which is why
        // the failure paths below call shutdown() explicitly and this one does
        // not.
        throw std::runtime_error("glfwInit failed (no display, or no windowing backend)");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window_ = glfwCreateWindow(config_.width, config_.height, config_.title.c_str(),
                               nullptr, nullptr);
    if (window_ == nullptr) {
        shutdown();  // glfwInit() succeeded, so that much has to be undone
        throw std::runtime_error("could not create a GL 3.3 core window (driver too old?)");
    }

    glfwMakeContextCurrent(window_);

    // 1 == swap on every vsync. Without it the loop below runs as fast as the
    // CPU allows and pins a core drawing frames nobody sees.
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    imgui_context_ = true;

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;  // no imgui.ini: a remembered layout is not worth a file

    // ImPlot keeps its own context and must be created after ImGui's, and
    // destroyed before it.
    ImPlot::CreateContext();
    implot_context_ = true;

    // Both backends return bool rather than throwing. A false here means the
    // context exists but no frame can ever be drawn, so unwinding now is the
    // difference between a clear message and an empty window.
    if (!ImGui_ImplGlfw_InitForOpenGL(window_, true)) {
        shutdown();
        throw std::runtime_error("ImGui_ImplGlfw_InitForOpenGL failed");
    }
    imgui_glfw_ = true;

    if (!ImGui_ImplOpenGL3_Init(kGlslVersion)) {
        shutdown();
        throw std::runtime_error("ImGui_ImplOpenGL3_Init failed");
    }
    imgui_opengl_ = true;

    // Preselect the strategy named on the command line, if it is one we know.
    // An unknown name leaves the index at 0 rather than throwing: the window is
    // allowed to open so the mistake is visible and correctable in the combo.
    for (int i = 0; i < kStrategyCount; ++i) {
        if (config_.strategy == kStrategyNames[i]) {
            strategy_index_ = i;
        }
    }

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.WindowPadding = ImVec2(10.0f, 10.0f);
}

GuiApp::~GuiApp() {
    shutdown();
}

void GuiApp::shutdown() noexcept {
    // Reverse order of construction: backends, then contexts, then the window,
    // then GLFW itself. Every step is guarded, so this is safe to call from a
    // half-built object -- which is exactly what the constructor's failure
    // paths do, and why each flag is cleared as its resource goes away.
    if (window_ != nullptr) {
        // The backend shutdowns below issue GL calls, and they are only valid
        // with the context current. Re-binding here is cheap and removes the
        // need to reason about whether some earlier path left a different
        // context bound.
        glfwMakeContextCurrent(window_);
    }
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

    // Safe to call even when no window was created, or when glfwInit() itself
    // failed -- GLFW documents it as a no-op on an uninitialised library.
    glfwTerminate();
}

int GuiApp::run() {
    while (glfwWindowShouldClose(window_) == 0 && !quit_requested_) {
        // Poll rather than wait for events: ImGui redraws every frame whatever
        // happened, so there is no idle state to block in.
        glfwPollEvents();

        // This order is not a style choice. ImGui asserts on it in debug
        // builds: both backend NewFrame calls, then ImGui::NewFrame, then any
        // ImGui:: call, then ImGui::Render, and only then the backend's
        // RenderDrawData.
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        draw_frame();

        ImGui::Render();

        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window_, &width, &height);

        // A minimised window has a zero-sized framebuffer. Skipping the draw is
        // not just an optimisation: glViewport(0, 0, 0, 0) followed by a draw
        // call is an error, and the frame would be rendered into nothing.
        if (width > 0 && height > 0) {
            glViewport(0, 0, width, height);
            glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        glfwSwapBuffers(window_);
    }

    return 0;
}

// ---------------------------------------------------------------------------
// Layout
//
// One undecorated window fills the viewport and everything else is a child
// region inside it. Docking would be the flexible answer, but it lives on a
// separate ImGui branch, so the split here is fixed: controls on the left at a
// set width, plot taking the rest.
// ---------------------------------------------------------------------------
void GuiApp::draw_frame() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    constexpr ImGuiWindowFlags host_flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;

    // Zero padding and no rounding so the host is flush with the window edges
    // and the children look like the top level, not like boxes in a box. The
    // three style vars are popped immediately after Begin, which is what the
    // ImGui demo does: Begin consumes them, the children must not inherit them.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("##host", nullptr, host_flags);
    ImGui::PopStyleVar(3);

    draw_menu_bar();

    // The status bar is measured first and the panels are sized to what is
    // left over, so a status line that wraps grows upward instead of pushing
    // the panels off the bottom edge.
    const float status_height = ImGui::GetFrameHeight();
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const float panels_height = available.y - status_height;

    constexpr float kControlsWidth = 320.0f;

    ImGui::BeginChild("##controls", ImVec2(kControlsWidth, panels_height),
                      ImGuiChildFlags_Borders);
    draw_controls_panel();
    ImGui::EndChild();

    ImGui::SameLine();

    // Width 0 means "whatever is left", which is how the plot absorbs the space
    // the fixed-width controls panel did not take.
    ImGui::BeginChild("##plot", ImVec2(0.0f, panels_height), ImGuiChildFlags_Borders);
    draw_plot_panel();
    ImGui::EndChild();

    draw_status_bar();

    ImGui::End();

    // Modal popups are not children of the host window; they are drawn at top
    // level, so this cannot live inside the Begin/End above. The matching
    // OpenPopup call sits on the menu item rather than here: calling it every
    // frame while the flag is set re-opens the popup each frame, which fights
    // the close button and resets the popup's focus state.
    if (ImGui::BeginPopupModal("About QuantSimulator", &show_about_,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("QuantSimulator");
        ImGui::Separator();
        ImGui::TextUnformatted("A C++ backtesting sandbox.");
        ImGui::Spacing();
        ImGui::Text("Dear ImGui %s", IMGUI_VERSION);
        ImGui::Text("ImPlot %s", IMPLOT_VERSION);
        ImGui::Spacing();
        if (ImGui::Button("Close", ImVec2(-FLT_MIN, 0.0f))) {
            show_about_ = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void GuiApp::draw_menu_bar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            // The accelerator is displayed but not bound. Binding it means
            // intercepting the key in the GLFW callback and deciding what
            // "Ctrl+Q while a text box has focus" should do, which is more
            // plumbing than a single menu item justifies.
            if (ImGui::MenuItem("Quit", "Ctrl+Q")) {
                quit_requested_ = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                ImGui::OpenPopup("About QuantSimulator");
                show_about_ = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

void GuiApp::draw_controls_panel() {
    ImGui::TextUnformatted("Data");
    ImGui::Separator();

    // InputText edits a raw char buffer, not a std::string, so the member is
    // copied in each frame and copied back only when the widget reports an
    // edit. This is the one place ImGui's C-style API shows through the C++
    // member state.
    char path_buffer[512];
    std::snprintf(path_buffer, sizeof(path_buffer), "%s", data_path_.c_str());
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::InputText("##data_path", path_buffer, sizeof(path_buffer))) {
        data_path_ = path_buffer;
    }
    ImGui::TextDisabled("csv to load");

    ImGui::Spacing();
    ImGui::TextUnformatted("Strategy");
    ImGui::Separator();

    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::BeginCombo("##strategy", kStrategyNames[strategy_index_])) {
        for (int i = 0; i < kStrategyCount; ++i) {
            const bool selected = (i == strategy_index_);
            if (ImGui::Selectable(kStrategyNames[i], selected)) {
                strategy_index_ = i;
            }
            // Puts the keyboard cursor on whichever entry is current, so the
            // combo opens usable rather than always at the top.
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Spacing();
    ImGui::TextUnformatted("Simulation");
    ImGui::Separator();

    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputDouble("##cash", &initial_cash_, 1000.0, 10000.0, "%.2f");
    ImGui::TextDisabled("initial cash");

    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputDouble("##commission", &commission_, 0.0005, 0.005, "%.5f");
    ImGui::TextDisabled("commission (fraction)");

    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputInt("##order_size", &order_size_, 10, 100);
    ImGui::TextDisabled("order size (shares)");
    if (order_size_ < 1) {
        order_size_ = 1;
    }

    ImGui::Spacing();
    ImGui::TextUnformatted("Engine");
    ImGui::Separator();

    // Disabled rather than absent, and it says why. A greyed control with a
    // reason beside it is a better hint than a missing one -- and it keeps the
    // intended shape of the panel visible while the engine is unwritten.
    //
    // TODO: drop the BeginDisabled/EndDisabled pair once Backtest::run() exists
    // and call it from here.
    ImGui::BeginDisabled();
    ImGui::Button("Run backtest", ImVec2(-FLT_MIN, 0.0f));
    ImGui::EndDisabled();

    ImGui::TextWrapped(
        "Not wired up: the engine is still stubbed, so there is nothing to run yet.");
}

void GuiApp::draw_plot_panel() {
    ImGui::TextUnformatted("Equity curve");
    ImGui::Separator();

    // A negative size means "fill what is left", so ImPlot takes the whole
    // remaining panel however the window is resized.
    if (ImPlot::BeginPlot("##equity", ImVec2(-1.0f, -1.0f))) {
        ImPlot::SetupAxes("bar", "equity");
        // Fixed limits rather than autofit: with no series plotted, autofit
        // has nothing to fit to and collapses the axes to a degenerate range.
        ImPlot::SetupAxesLimits(0.0, 1.0, 0.0, 1.0, ImPlotCond_Always);

        // The annotation, not the empty frame, is what distinguishes "no data
        // yet" from "broken". Delete it along with the limits above once there
        // is a series to draw.
        ImPlot::PlotText("no data - the engine is not wired up yet", 0.5, 0.5);

        // TODO: once Backtest::equity_curve() returns a series, plot it here:
        //   ImPlot::PlotLine("equity", xs.data(), equity.data(), (int)equity.size());
        // with xs a std::vector<double> of bar indices, which PlotLine requires
        // as doubles even though the x axis is conceptually an int index.

        ImPlot::EndPlot();
    }
}

void GuiApp::draw_status_bar() {
    ImGui::Separator();
    ImGui::Text("status: %s", status_.c_str());

    // Right-aligned in a fixed slot, so the number does not wander as its own
    // width changes and the eye can find it in the same place every frame.
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 110.0f);
    ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
}

}  // namespace quant::ui
