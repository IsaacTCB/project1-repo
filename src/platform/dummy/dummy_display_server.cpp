#include "../display_server.h"

#include "dummy_common.h"

void DisplayServer::init_display(uint32_t width, uint32_t height, std::string_view caption) {
    return;
}

std::tuple<uint32_t, uint32_t> DisplayServer::get_size() const {
    return { 0u, 0u };
}
