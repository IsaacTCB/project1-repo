#pragma once

#include "imgui_profiler.h"

#include <cstdint>
#include <limits>
#include <source_location>

#include <imgui.h>

/* types used to build the node tree data structure */
namespace ImGuiProfilerTypes {

using NodeID = ImGuiProfiler::NodeID;
static constexpr auto NO_NODE = NodeID{};

enum class NodeKind : uint8_t {
    ROOT = 0,
    THREAD,
    SCOPE,
};

enum class SortKind : uint8_t {
    CHRONOLOGICAL = 0,
    TOTAL_TIME,
    SELF_TIME,
};

struct NodeDef {
    NodeKind kind;

    uint64_t thread_id = 0;
    bool is_main_thread = false;

    /* intrusive linked list */
    uint64_t first_start_ns{std::numeric_limits<uint64_t>::max()};
    uint64_t last_end_ns{};
    uint64_t self_ns{};
    uint64_t min_ns{std::numeric_limits<uint64_t>::max()};
    uint64_t max_ns{};
    uint64_t total_ns{};
    NodeID first_child = NO_NODE;
    NodeID sibling = NO_NODE;

    std::source_location source{};
    size_t calls{};

    /* children */
    size_t children_begin{};
    size_t children_count{};
};

struct ScopeData {
    uint64_t end_ns;
    NodeID node;
};

} /* namespace ImGuiProfilerTypes */

/* helper functions used by the profiler window */
namespace ImGuiProfilerHelpers {

inline void text_right_aligned(const char *text) {
    float width = ImGui::CalcTextSize(text).x;
    float avail_width = ImGui::GetContentRegionAvail().x;
    if (avail_width > width) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail_width - width));
    }
    ImGui::TextUnformatted(text);
}

inline int format_duration_string(uint64_t time_ns, char *buffer, size_t buffer_size) {
    if (time_ns >= 1'000'000) { /* millisecond */
        return std::snprintf(buffer, buffer_size, "%.1fms", static_cast<double>(time_ns) / 1e6);
    }
    if (time_ns >= 1'000) { /* microsecond */
        return std::snprintf(buffer, buffer_size, "%luus", time_ns / static_cast<uint64_t>(1e3));
    }
    /* nanosecond */
    return std::snprintf(buffer, buffer_size, "%luns", time_ns);
}

/**
 * @brief Displays fraction as percentage with a progress bar behind it.
 *
 * @param fraction Normalized value from 0.0 to 1.0
 */
inline void fraction_cell(float fraction) {
    fraction = std::clamp(fraction, 0.0f, 1.0f);

    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImVec2 size(
        ImGui::GetContentRegionAvail().x,
        ImGui::GetTextLineHeight());

    ImGui::GetWindowDrawList()
        ->AddRectFilled(
            pos, ImVec2(pos.x + (size.x * fraction), pos.y + size.y),
            ImGui::GetColorU32(ImGuiCol_PlotHistogram, 0.45f));

    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.1f%%", fraction * 100.0f);
    text_right_aligned(buffer);
}

} /* namespace ImGuiProfilerHelpers */
