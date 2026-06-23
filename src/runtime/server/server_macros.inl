#pragma once

#define DECLARE_SERVER(Server)                  \
public:                                         \
    explicit Server() = default;                \
    Server(const Server &other) = delete;       \
    Server &operator=(Server &other) = delete;  \
    Server(Server &&other) = delete;            \
    Server &operator=(Server &&other) = delete; \
                                                \
private:
