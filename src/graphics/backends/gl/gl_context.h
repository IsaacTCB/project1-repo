#pragma once

#include "core/math/vec2i.h"

#include <string_view>

class GL_IContext {
public:
    using FnProcAddr = void (*)();
    using FnLoad = FnProcAddr (*)(const char *);

public:
    virtual ~GL_IContext() = default;

    /**
     * @brief Makes this GL context current
     */
    virtual void bind() = 0;

    [[nodiscard]]
    virtual FnLoad get_load_proc() = 0;

    /**
     * @brief Swap window screen buffers
     */
    virtual void swap_buffers() = 0;

    /**
     * @brief Set swap interval time. This function can be used for enabling v-sync.
     *
     * @param interval Typical values are 0 to disable, 1 for vsync.
     */
    virtual void set_swap_interval(int interval) = 0;

    /**
     * @brief Checks if an extension is supported
     *
     * @param extension_name What extension to check
     * @return Returns true if supported, false if not.
     */
    virtual bool is_extension_supported(std::string_view extension_name) = 0;

    virtual Vec2i get_screen_size() = 0;

protected:
    GL_IContext() = default;
};
