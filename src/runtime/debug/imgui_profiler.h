#pragma once

#include <memory>
#include <span>

#include "core/debug/profiler/profiler.h"

class ImGuiProfiler {
public:
    using NodeID = uint32_t;

public:
    explicit ImGuiProfiler();
    ~ImGuiProfiler();

    void draw(const char *title = "Profiler", bool *open = nullptr);
    void push_frame(const ProfilerFrame &frame);

private:
    [[nodiscard]]
    std::span<NodeID> children_of(NodeID n) const;
    // void print_indentation(uint32_t node, size_t indent = 0) const;

    void draw_controls();
    void draw_frame_summary();
    void draw_scopes();

    /* recursive function creates a tree
     * for the node and its children */
    void draw_scope_tree(NodeID node, size_t recursion = 0);

    void rebuild_tree(const ProfilerFrame &frame);
    void sort_tree();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
