#include "imgui_profiler.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <numeric>
#include <source_location>
#include <vector>

#include "core/debug/profiler/profiler.h"
#include "core/debug/profiler/profiler_limits.h"

#include <imgui.h>

#include "imgui_profiler_internal.h"

/* I don't want to type these verbose names all the time... */
using namespace ImGuiProfilerTypes;
using namespace ImGuiProfilerHelpers;

constexpr ImColor MAIN_THREAD_COLOR(66, 135, 245);
constexpr ImColor WORKER_THREAD_COLOR(245, 188, 66);
constexpr ImColor TEXT_WARNING_COLOR(245, 188, 66);
constexpr ImColor SCOPE_TAGGED_COLOR(190, 250, 233);

struct ImGuiProfiler::Impl {
    std::vector<NodeDef> nodes;
    std::vector<NodeID> children;
    std::vector<ScopeData> scope_stack;
    std::vector<size_t> event_order;

    bool paused = false;
    bool short_name = true;
    SortKind sort_kind{SortKind::CHRONOLOGICAL};
    size_t dropped{};
};

/* ================= *
 *      Public       *
 * ================= */

ImGuiProfiler::ImGuiProfiler() {
    m_impl = std::make_unique<Impl>();

    /* pre-allocate all arrays */
    m_impl->nodes.reserve(ProfilerLimits::MAX_EVENTS_PER_FRAME);
    m_impl->children.reserve(ProfilerLimits::MAX_EVENTS_PER_FRAME);
    m_impl->event_order.reserve(ProfilerLimits::MAX_EVENTS_PER_FRAME);
    m_impl->scope_stack.resize(32);
}

ImGuiProfiler::~ImGuiProfiler() = default;

void ImGuiProfiler::draw(const char *title, bool *open) {
    PROFILE_FUNCTION_SCOPE();

    if (!ImGui::Begin(title, open)) {
        ImGui::End();
        return;
    }

    draw_controls();

    if (m_impl->nodes.empty()) {
        ImGui::Text("Waiting for profiler data...");
        ImGui::End();
        return;
    }

    draw_frame_summary();
    draw_scopes();

    ImGui::End();
}

void ImGuiProfiler::push_frame(const ProfilerFrame &frame) {
    /* we only update the tree if we aren't paused. */
    if (!m_impl->paused) {
        rebuild_tree(frame);
    }

    /* TODO: this sorts on every frame, even while paused.
     * if that's the case, then make this re-sort only when
     * the user updates the sort kind (unpaused remains
     * the same) */
    sort_tree();
}

/* ==================== *
 *     ImGui window     *
 * ==================== */

static void node_tooltip(const NodeDef &node) {
    char buffer[32];
    if (node.kind == NodeKind::SCOPE) {
        const bool is_tagged = node.tag != nullptr;
        if (is_tagged) {
            ImGui::TextColored(SCOPE_TAGGED_COLOR, "%s", node.tag);
            ImGui::TextDisabled("%s", node.source.function_name());
        } else {
            ImGui::TextUnformatted(node.source.function_name());
        }
        ImGui::TextDisabled("%s:%d", node.source.file_name(), node.source.line());

        ImGui::Separator();

        format_duration_string(node.total_ns, buffer, sizeof(buffer));
        ImGui::Text("Total: %s", buffer);
        format_duration_string(node.self_ns, buffer, sizeof(buffer));
        ImGui::Text("Self:  %s", buffer);

        if (node.calls > 1) {
            ImGui::Separator();
            ImGui::Text("Calls: %zu", node.calls);
            format_duration_string(node.total_ns / node.calls, buffer, sizeof(buffer));
            ImGui::Text("Avg:   %s", buffer);
            format_duration_string(node.min_ns, buffer, sizeof(buffer));
            ImGui::Text("Min:   %s", buffer);
            format_duration_string(node.max_ns, buffer, sizeof(buffer));
            ImGui::Text("Max:   %s", buffer);
        }

    } else {
        if (node.is_main_thread) {
            ImGui::TextColored(MAIN_THREAD_COLOR, "Main Thread");
        } else {
            ImGui::TextUnformatted("Thread");
        }
        ImGui::TextDisabled("id 0x%zx", node.thread_id);
        ImGui::Separator();

        format_duration_string(node.total_ns, buffer, sizeof(buffer));
        ImGui::Text("Span:      %s", buffer);
        format_duration_string(node.self_ns, buffer, sizeof(buffer));
        ImGui::Text("Untracked: %s", buffer);
        ImGui::Text("Events:    %zu", node.calls);

        ImGui::Separator();
        ImGui::TextDisabled("Spans runs from the first scope start to the last scope end on this thread.\n"
                            "Untracked is the part of it that haven't been covered by any of its scopes.");
    }
}

void ImGuiProfiler::draw_controls() {
    ImGui::Checkbox("Paused", &m_impl->paused);
    if (ImGui::BeginItemTooltip()) {
        ImGui::Text("Pauses the profiler stack tree. The last\n"
                    "profiler frame stays in a frozen state.");
        ImGui::EndTooltip();
    }

    ImGui::SameLine();
    ImGui::Checkbox("Short Names", &m_impl->short_name);

    static const char *sort_names[] = {
        "Chronological",
        "Total Time",
        "Self Time",
    };
    const auto sort_selected = static_cast<size_t>(m_impl->sort_kind);
    ImGui::SameLine();
    ImGui::PushItemWidth(ImGui::CalcTextSize("Chronological___Sort").x);
    if (ImGui::BeginCombo("Sort", sort_names[sort_selected])) {
        for (size_t n = 0; n < IM_ARRAYSIZE(sort_names); n++) {
            const bool is_selected = (sort_selected == n);
            if (ImGui::Selectable(sort_names[n], is_selected)) {
                m_impl->sort_kind = static_cast<SortKind>(n);
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    if (ImGui::BeginItemTooltip()) {
        ImGui::Text("Dictates the profiler stack tree node\n"
                    "order.");
        ImGui::EndTooltip();
    }
}

void ImGuiProfiler::draw_frame_summary() {
    char buffer[64];

    ImGui::SeparatorText("Frame");

    ImGui::TextUnformatted("Duration: ");
    ImGui::SameLine();
    format_duration_string(m_impl->nodes.front().total_ns, buffer, sizeof(buffer));
    ImGui::TextUnformatted(buffer);

    ImGui::SameLine();
    ImGui::Text("  Threads: %zu", m_impl->nodes.front().children_count);

    ImGui::SameLine();
    if (m_impl->dropped == 0) {
        ImGui::TextDisabled("  Dropped: 0");
    } else {
        ImGui::TextColored(TEXT_WARNING_COLOR,
            "  Dropped: %zu", m_impl->dropped);
    }
    ImGui::SetItemTooltip(
        "The number of events that the profiler has skipped.\n"
        "Events get dropped whenever the event limit (%zu) is reached.",
        ProfilerLimits::MAX_EVENTS_PER_FRAME);
}

void ImGuiProfiler::draw_scopes() {
    ImGui::SeparatorText("Scopes");

    if (!ImGui::BeginChild("profiler_subwindow")) {
        ImGui::EndChild();
        return;
    }

    /* setup table */
    if (!ImGui::BeginTable("profiler_table", 6, ImGuiTableFlags_BordersV | ImGuiTableFlags_RowBg)) {
        ImGui::EndChild();
        return;
    }
    ImGui::TableSetupScrollFreeze(0, 1);

    const float duration_width = ImGui::CalcTextSize("000us_").x;
    ImGui::TableSetupColumn("Scope", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Total", ImGuiTableColumnFlags_WidthFixed, duration_width);
    ImGui::TableSetupColumn("Self", ImGuiTableColumnFlags_WidthFixed, duration_width);
    ImGui::TableSetupColumn("% Frame", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("% Frame___").x);
    ImGui::TableSetupColumn("Calls", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Calls_").x);
    ImGui::TableSetupColumn("Location", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("imgui_profiler.cpp:000").x);
    ImGui::TableHeadersRow();

    /* begin drawing each thread tree */
    if (!m_impl->children.empty()) {
        for (NodeID thread : children_of(NodeID{})) {
            draw_scope_tree(thread);
        }
    }

    ImGui::EndTable();
    ImGui::EndChild();
}

void ImGuiProfiler::draw_scope_tree(NodeID node, size_t recursion) {
    char buffer[32];

    const auto &node_def = m_impl->nodes[node];
    const bool is_leaf = node_def.children_count == 0;
    const bool is_thread = node_def.kind == NodeKind::THREAD;

    ImGui::PushID(node);
    ImGui::TableNextRow();

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_SpanFullWidth |
        ImGuiTreeNodeFlags_OpenOnDoubleClick |
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_DrawLinesToNodes;

    /* the first 2 levels will be open by default. */
    if (recursion < 2) {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    if (is_leaf) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    ImGui::TableSetColumnIndex(0);
    bool open = false;

    if (is_thread && node_def.is_main_thread) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImU32{MAIN_THREAD_COLOR});
        open = ImGui::TreeNodeEx("Main Thread", flags, "Main Thread");
        ImGui::PopStyleColor();
    } else if (is_thread) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImU32{WORKER_THREAD_COLOR});
        open = ImGui::TreeNodeEx("Thread", flags, "Thread 0x%zx", node_def.thread_id);
        ImGui::PopStyleColor();
    } else {
        const bool is_tagged = node_def.tag != nullptr;
        if (is_tagged) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImU32{SCOPE_TAGGED_COLOR});
            open = ImGui::TreeNodeEx(node_def.tag, flags, "%s", node_def.tag);
            ImGui::PopStyleColor();
        } else {
            const char *func = node_def.source.function_name();
            const auto f = m_impl->short_name
                             ? short_function_name(func)
                             : std::string_view{func};
            open = ImGui::TreeNodeEx(func, flags, "%.*s", static_cast<int>(f.size()), f.data());
        }
    }

    /* 1. scopes */
    if (ImGui::BeginItemTooltip()) {
        node_tooltip(node_def);
        ImGui::EndTooltip();
    }

    /* 2. total */
    ImGui::TableNextColumn();
    format_duration_string(node_def.total_ns, buffer, sizeof(buffer));
    text_right_aligned(buffer);

    /* 3. self */
    ImGui::TableNextColumn();
    format_duration_string(node_def.self_ns, buffer, sizeof(buffer));
    text_right_aligned(buffer);

    /* 4. frame % */
    const uint64_t frame_total = m_impl->nodes.front().total_ns;
    const double frac = frame_total > 0 ? static_cast<double>(node_def.total_ns) / static_cast<double>(frame_total) : 0.0f;
    ImGui::TableNextColumn();
    fraction_cell(static_cast<float>(frac));

    /* 5. calls % */
    ImGui::TableNextColumn();
    if (!is_thread) {
        std::snprintf(buffer, sizeof(buffer), "%d", static_cast<uint32_t>(node_def.calls));
        text_right_aligned(buffer);
    }

    /* 6. location % */
    ImGui::TableNextColumn();
    if (!is_thread) {
        const auto filename =
            std::filesystem::path{node_def.source.file_name()}
                .filename()
                .generic_string();
        ImGui::TextDisabled("%s:%d", filename.data(), node_def.source.line());
    }

    if (open) {
        /* begin children subtree */
        for (NodeID child : children_of(node)) {
            draw_scope_tree(child, recursion + 1);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

/* ================== *
 *    Tree builder    *
 * ================== */

static bool is_same_scope(const ProfilerEvent &event, const NodeDef &node_def) {
    return (event.tag == node_def.tag &&
            event.source.line() == node_def.source.line() &&
            event.source.function_name() == node_def.source.function_name() &&
            event.source.file_name() == node_def.source.file_name());
}

void ImGuiProfiler::rebuild_tree(const ProfilerFrame &frame) {
    /* clear all vectors without reallocating them */
    m_impl->nodes.clear();
    m_impl->children.clear();
    m_impl->scope_stack.clear();
    m_impl->event_order.clear();

    m_impl->dropped = frame.dropped;

    /* node 0 is always the root node */
    m_impl->nodes.push_back(NodeDef{.kind = NodeKind::ROOT});

    /* find out the correct event order */
    m_impl->event_order.resize(frame.events.size());
    std::ranges::iota(m_impl->event_order, 0);
    std::ranges::sort(m_impl->event_order, [&](size_t i1, size_t i2) {
        const auto &a = frame.events[i1];
        const auto &b = frame.events[i2];

        /* threads should be contiguous */
        if (a.thread_id != b.thread_id) {
            return a.thread_id < b.thread_id;
        }
        if (a.start_ns != b.start_ns) {
            return a.start_ns < b.start_ns;
        }
        return a.duration_ns > b.duration_ns;
    });

    /* first pass: build profiler call-stack tree
     *
     * you can imagine the profiler tree like so:
     *    root
     *    | - (thread #1)
     *    |   | - function()
     *    |   | - function2()
     *    |   |   | - nested_function()
     *    |
     *    | - (thread #2)
     *    |   | - function3()
     */

    NodeID thread_node = NO_NODE;

    /* utility lambdas, should get compiled off on release builds */
    const auto def = [&](NodeID id) -> auto & { return m_impl->nodes[static_cast<size_t>(id)]; };
    const auto thread_def = [&]() -> auto & { return def(thread_node); };
    const auto root_def = [&]() -> auto & { return def(0); };

    for (const auto i : m_impl->event_order) {
        const auto &event = frame.events[i];
        const uint64_t end_ns = event.start_ns + event.duration_ns;

        const bool is_main_thread = event.thread_id == frame.main_thread_id;

        /* if this event from a new thread, then create a new thread tree */
        if (thread_node == NO_NODE || thread_def().thread_id != event.thread_id) {
            thread_node = static_cast<NodeID>(m_impl->nodes.size());
            m_impl->nodes.push_back(NodeDef{
                .kind = NodeKind::THREAD,
                .thread_id = event.thread_id,
                .is_main_thread = is_main_thread,

                .first_start_ns = event.start_ns,
                .first_child = NO_NODE,
                .sibling = root_def().first_child,
            });
            root_def().first_child = thread_node;
            m_impl->scope_stack.clear();
        }

        auto &thread = thread_def();
        thread.last_end_ns = std::max(thread.last_end_ns, end_ns);
        thread.calls++;

        /* pop scopes whose lifetime ended */
        while (!m_impl->scope_stack.empty() && end_ns > m_impl->scope_stack.back().end_ns) {
            m_impl->scope_stack.pop_back();
        }
        NodeID parent_node = m_impl->scope_stack.empty()
                               ? thread_node
                               : m_impl->scope_stack.back().node;
        auto &parent = def(parent_node);

        /* merge with sibling if it's the same scope */
        NodeID node_idx = NO_NODE;
        for (NodeID child = parent.first_child; child != NO_NODE; child = def(child).sibling) {
            if (is_same_scope(event, def(child))) {
                node_idx = child;
                break;
            }
        }

        /* no sibling found, this scope is new */
        if (node_idx == NO_NODE) {
            node_idx = static_cast<NodeID>(m_impl->nodes.size());
            m_impl->nodes.push_back(NodeDef{
                .kind = NodeKind::SCOPE,
                .thread_id = event.thread_id,
                .is_main_thread = is_main_thread,
                .first_start_ns = event.start_ns,
                .first_child = NO_NODE,
                .sibling = parent.first_child,

                .tag = event.tag,
                .source = event.source,
            });
            parent.first_child = node_idx;
        }

        /* update durations */
        auto &node = m_impl->nodes[node_idx];
        node.total_ns += event.duration_ns;
        node.min_ns = std::min(node.min_ns, event.duration_ns);
        node.max_ns = std::max(node.max_ns, event.duration_ns);
        node.calls++;

        m_impl->scope_stack.push_back({.end_ns = end_ns, .node = node_idx});
    }

    /* second pass: patch root and thread nodes to set their total duration time */
    auto &root = m_impl->nodes.front();
    for (NodeID n = root.first_child; n != NO_NODE; n = m_impl->nodes[n].sibling) {
        auto &thread = m_impl->nodes[n];
        thread.total_ns = thread.last_end_ns - thread.first_start_ns;

        root.first_start_ns = std::min(root.first_start_ns, thread.first_start_ns);
        root.last_end_ns = std::max(root.last_end_ns, thread.last_end_ns);
    }
    root.total_ns = root.last_end_ns - root.first_start_ns;

    /* final pass: iterate through every node to create flatten children array,
     * and also to calculate self_ns */
    for (auto &node : m_impl->nodes) {
        node.children_begin = m_impl->children.size();

        uint64_t children_ns = 0;
        for (NodeID n = node.first_child; n != NO_NODE; n = m_impl->nodes[n].sibling) {
            m_impl->children.push_back(n);
            children_ns += m_impl->nodes[n].total_ns;
        }
        node.children_count = m_impl->children.size() - node.children_begin;
        node.self_ns = node.total_ns > children_ns
                         ? (node.total_ns - children_ns)
                         : 0;
    }
}

/* ============= *
 *     Misc.     *
 * ============= */

void ImGuiProfiler::sort_tree() {
    switch (m_impl->sort_kind) {
        /* sort things cronologically, based on their start timestamp */
        case SortKind::CHRONOLOGICAL:
            for (const auto node : m_impl->children) {
                std::ranges::sort(children_of(node), [&](NodeID a, NodeID b) {
                    const auto &lhs = m_impl->nodes[a];
                    const auto &rhs = m_impl->nodes[b];
                    return lhs.first_start_ns < rhs.first_start_ns;
                });
            }
            break;

        /* sort events using total time */
        case SortKind::TOTAL_TIME:
            for (const auto node : m_impl->children) {
                std::ranges::sort(children_of(node), [&](NodeID a, NodeID b) {
                    const auto &lhs = m_impl->nodes[a];
                    const auto &rhs = m_impl->nodes[b];
                    return lhs.total_ns > rhs.total_ns;
                });
            }
            break;

        /* sort events using self time */
        case SortKind::SELF_TIME:
            for (const auto node : m_impl->children) {
                std::ranges::sort(children_of(node), [&](NodeID a, NodeID b) {
                    const auto &lhs = m_impl->nodes[a];
                    const auto &rhs = m_impl->nodes[b];
                    return lhs.self_ns > rhs.self_ns;
                });
            }
            break;
    }
}

std::span<NodeID> ImGuiProfiler::children_of(NodeID n) const {
    const auto &def = m_impl->nodes[n];
    return std::span{m_impl->children}.subspan(def.children_begin, def.children_count);
}

// void ImGuiProfiler::print_indentation(NodeID node, size_t indent) const {
//     for (auto n : children_of(node)) {
//         auto &def = m_impl->nodes[n];
//         const bool is_leaf = def.children_count == 0;
//
//         for (size_t i = 0; i < indent; ++i) {
//             std::cout << "| ";
//         }
//
//         std::cout << "- ";
//         switch (def.kind) {
//             case NodeKind::SCOPE:
//                 std::cout << def.source.function_name();
//                 break;
//             case NodeKind::THREAD:
//                 std::cout << "THR: " << std::format("{:#X}", def.thread_id);
//                 break;
//             default:
//                 break;
//         }
//         std::cout << "  (d=" << (def.self_ns / 1000) << ", t=" << (def.total_ns / 1000) << ")\n";
//
//         if (!is_leaf) {
//             print_indentation(n, indent + 1);
//         }
//     }
// }
