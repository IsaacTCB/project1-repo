#pragma once

class IImGuiBackend {
public:
    virtual ~IImGuiBackend() = default;

    /**
     * @brief Called to init ImGui backend
     *
     * @return true if backend was initialized successfully.
     */
    virtual bool init() = 0;

    /**
     * @brief Called on closure to shutdown ImGui backend.
     */
    virtual void shutdown() = 0;

    /**
     * @brief Opens a new ImGui frame window.
     */
    virtual void new_frame() = 0;

    /**
     * @brief Submits ImGui::GetDrawData() to the GPU.
     *
     * Called before swapping buffers.
     */
    virtual void render() = 0;
};

IImGuiBackend *imgui_create_null_backend();
