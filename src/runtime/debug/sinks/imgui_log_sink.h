#pragma once

#include "core/logger.h"

#include <cstddef>
#include <deque>
#include <memory>
#include <string>
#include <string_view>

class ImGuiLogState {
public:
    struct Line {
        std::string string;
        LogSeverity severity;
    };

public:
    explicit ImGuiLogState(size_t line_count = 2000)
        : m_max_lines(line_count), m_generation(0) {}

    ImGuiLogState() = delete;

    void push_line(LogSeverity severity, std::string_view log) {
        m_deque.emplace_back(Line{
            .string = std::string{log},
            .severity = severity,
        });
        while (m_deque.size() >= m_max_lines) {
            m_deque.pop_front();
        }

        m_generation++;
    }

    void clear() {
        m_deque.clear();
        m_generation++;
    }

    [[nodiscard]]
    size_t size() const {
        return m_deque.size();
    }

    [[nodiscard]]
    size_t generation() const {
        return m_generation;
    }

    template <typename Func>
    void for_each(Func &&func) {
        for (const Line &line : m_deque) {
            func(line);
        }
    }

private:
    std::deque<Line> m_deque;
    size_t m_max_lines;
    size_t m_generation;
};

namespace ImGuiLogSink {

std::unique_ptr<ILogSink> create(std::shared_ptr<ImGuiLogState> state);

} /* namespace ImGuiLogSink */
