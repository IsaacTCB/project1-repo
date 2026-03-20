#pragma once

#include <memory>

class GLContext {
public:
    using FnProcAddr = void(*)();
    using FnLoad = FnProcAddr(*)(const char*);
public:
    GLContext();
    ~GLContext();

    void init(void *native_window);
    void close();

    /**
     * @brief Makes this GL context current
     */
    void bind();

    /**
     * @brief Swap window screen buffers
     */
    void swap_buffers();

    /**
     * @brief Set swap interval time. This function can be used for enabling v-sync.
     *
     * @param interval Typical values are 0 to disable, 1 for vsync.
     */
    void set_swap_interval(int interval);

    FnLoad const load_func;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
