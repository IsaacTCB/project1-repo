#pragma once

#include "runtime/server/io_server.h"

class SDL3_IOServer final : public IIOServer {
public:
    void open_file() override {}
};
