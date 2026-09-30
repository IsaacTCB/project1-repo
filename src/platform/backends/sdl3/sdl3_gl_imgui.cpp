#include "sdl3_gl_imgui.h"

#include <SDL3/SDL_video.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl3.h>

class SDL3_GL_ImGuiBackend final : public IImGuiBackend {
public:
    explicit SDL3_GL_ImGuiBackend(SDL_Window *window)
        : m_window(window) {}

    bool init() override {
        if (!ImGui_ImplSDL3_InitForOpenGL(m_window, SDL_GL_GetCurrentContext())) {
            return false;
        }
        return ImGui_ImplOpenGL3_Init("#version 330 core");
    }

    void shutdown() override {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
    }

    void new_frame() override {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
    }

    void render() override {
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

private:
    SDL_Window *m_window;
};

IImGuiBackend *sdl3_gl_create_imgui(SDL_Window *window) {
    return new SDL3_GL_ImGuiBackend(window);
}
