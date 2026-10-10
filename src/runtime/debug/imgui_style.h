#pragma once

struct ImGuiStyle;

namespace ImGuiCustomStyles {

/**
 * @brief Applies a custom dark mode style for ImGui.
 *
 * Taken from the public ImGui style thread:
 * Originally created by Raikiri, then updated by Sixze.
 *
 * https://github.com/ocornut/imgui/issues/707#issuecomment-4121907946
 *
 * @param style The ImGuiStyle for applying the style (typically ImGui::GetStyle())
 */
void apply_dark_mode(ImGuiStyle &style);

}; // namespace ImGuiCustomStyles
