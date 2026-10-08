#include "file_log_sink.h"

#include <fstream>

namespace {

class File_LogSink : public ILogSink {
public:
    [[nodiscard]]
    std::span<const LogFeature> feature() const override {
        static constexpr LogFeature features[] = {
            LogFeature::DATETIME,
            LogFeature::SEVERITY,
            LogFeature::ARROW,
        };
        return features;
    }

    bool open(const std::filesystem::path &filepath) {
        m_file.open(filepath);
        return m_file.is_open() && m_file.good();
    }

    void push_line([[maybe_unused]] LogSeverity severity, std::string_view string) override {
        m_file << string << "\n";
    }

    void flush() override {
        m_file.flush();
    }

private:
    std::ofstream m_file;
};

} /* namespace */

namespace FileLogSink {

std::unique_ptr<ILogSink> open(const std::filesystem::path &filepath) {
    auto sink = std::make_unique<File_LogSink>();
    if (sink && sink->open(filepath)) {
        return std::move(sink);
    }
    return {};
}

}; /* namespace FileLogSink */
