#pragma once

#include "core/logger.h"

#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

class ImGuiLogState {
public:
    static constexpr size_t DEFAULT_LINE_COUNT = 2000;

    struct Line {
        std::string string;
        LogSeverity severity;
    };

public:
    explicit ImGuiLogState(size_t line_count = DEFAULT_LINE_COUNT)
        : m_max_lines(line_count), m_generation(0) {}

    ImGuiLogState() = delete;

    void push_line(LogSeverity severity, std::string_view log) {
        auto lock = std::scoped_lock(m_mutex);

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
        auto lock = std::scoped_lock(m_mutex);

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
        auto lock = std::scoped_lock(m_mutex);
        for (const Line &line : m_deque) {
            func(line);
        }
    }

private:
    std::deque<Line> m_deque;
    std::mutex m_mutex;
    size_t m_max_lines;
    size_t m_generation;
};

namespace ImGuiLogSink {

std::unique_ptr<ILogSink> create(std::shared_ptr<ImGuiLogState> state);

} /* namespace ImGuiLogSink */
