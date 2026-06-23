#pragma once

#include "server_macros.inl"

class IIOServer {
    DECLARE_SERVER(IIOServer)

public:
    virtual void open_file() = 0;
};
