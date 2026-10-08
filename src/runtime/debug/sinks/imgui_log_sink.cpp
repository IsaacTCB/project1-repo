#include "imgui_log_sink.h"

#include <memory>
#include <utility>

namespace {

class ImGui_LogSink : public ILogSink {
public:
    explicit ImGui_LogSink(std::shared_ptr<ImGuiLogState> state)
        : m_state(std::move(state)) {}

    [[nodiscard]]
    std::span<const LogFeature> feature() const override {
        static constexpr LogFeature features[] = {
            LogFeature::TIME,
            LogFeature::SEVERITY,
        };
        return features;
    }

    void push_line(LogSeverity severity, std::string_view string) override {
        m_state->push_line(severity, string);
    }

    void flush() override {}

private:
    std::shared_ptr<ImGuiLogState> m_state;
};

}; // namespace

namespace ImGuiLogSink {

std::unique_ptr<ILogSink> create(std::shared_ptr<ImGuiLogState> state) {
    return std::make_unique<ImGui_LogSink>(std::move(state));
}

} /* namespace ImGuiLogSink */
