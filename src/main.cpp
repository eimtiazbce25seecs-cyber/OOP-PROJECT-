#include "ui/gui.hpp"

int main() {
    quant::ui::GuiConfig config;
    quant::ui::GuiApp app(config);
    return app.run();
}
