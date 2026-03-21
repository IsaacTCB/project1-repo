#pragma once

class GLContext {
public:
    void make_current();
    void swap_buffers();

protected:
    static inline bool s_loaded = false;
};
