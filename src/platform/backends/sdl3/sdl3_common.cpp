#include "sdl3_common.h"

#include <SDL3/SDL_log.h>

#include "core/logger.h"

#include <iostream>

class SDL3_LogSink : public ILogSink {
public:
    [[nodiscard]]
    std::span<const LogFeature> feature() const override {
        static constexpr LogFeature features[] = {
            LogFeature::TIME,
            LogFeature::SEVERITY,
        };
        return features;
    }

    void push_line([[maybe_unused]] LogSeverity severity, std::string_view string) override {
#ifndef EMSCRIPTEN
        SDL_Log("%s\n", string.data());
#else
        std::cout << string << "\n";
#endif
    }

    void flush() override {
        std::cout.flush();
    }
};

void log_init() {
    Logger::get().set_sink_override(std::make_unique<SDL3_LogSink>());
}
