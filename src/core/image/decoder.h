#pragma once

#include "image.h"

#include <string_view>

namespace Image {
ImageData decode(std::string_view path);
}
