#include "platform/api/display_server.h"

#include "core/assertion.h"
#include "core/logger.h"
#include "psp_common.h"

DisplayWindowHandle DisplayServer::init_display(const DisplayWindowProps &props) {
    if (m_state.has_open_display) {
        /* Only allows one display to be open at all times. */
        return {};
    }

    m_state.has_open_display = true;

    return DisplayWindowHandle{
        .raw_window_handle = &m_state.has_open_display,
        .props = props,
    };
}

void DisplayServer::close_display(DisplayWindowHandle handle) {
    /* HACK: if the user passes an arbitrary handle value, it
     * will still change the variable */
    if (handle) {
        m_state.has_open_display = false;
    }
}

Vec2i DisplayServer::get_size([[maybe_unused]] const DisplayWindowHandle &handle) const {
    /* psp screens have a fixed screen size. */
    return {480, 272};
}
