#include "imgui_backend.h"

#include <backends/imgui_impl_null.h>

class Null_ImGuiBackend final : public IImGuiBackend {
public:
    bool init() override {
        return ImGui_ImplNull_Init();
    }

    void shutdown() override {
        ImGui_ImplNull_Shutdown();
    }

    void new_frame() override {
        ImGui_ImplNull_NewFrame();
    }

    void render() override {}
};

IImGuiBackend *imgui_create_null_backend() {
    return new Null_ImGuiBackend();
}
