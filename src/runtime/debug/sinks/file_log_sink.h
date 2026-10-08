#pragma once

#include "core/logger.h"

#include <filesystem>
#include <memory>

namespace FileLogSink {

std::unique_ptr<ILogSink> open(const std::filesystem::path &filepath);

}; /* namespace FileLogSink */
