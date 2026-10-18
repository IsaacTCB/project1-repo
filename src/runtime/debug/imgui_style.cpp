#include "imgui_style.h"

#include <imgui.h>

static ImVec4 adjust_hue_saturation(const ImVec4 &Color, const float HueShift, const float SaturationScale) {
    ImVec4 ResultColor = Color;
    ImGui::ColorConvertRGBtoHSV(ResultColor.x, ResultColor.y, ResultColor.z, ResultColor.x, ResultColor.y, ResultColor.z);

    ResultColor.x += HueShift;
    ResultColor.y *= SaturationScale;

    ImGui::ColorConvertHSVtoRGB(ResultColor.x, ResultColor.y, ResultColor.z, ResultColor.x, ResultColor.y, ResultColor.z);
    return ResultColor;
}

void ImGuiCustomStyles::apply_dark_mode(::ImGuiStyle &style) {
    /* Taken from: https://github.com/ocornut/imgui/issues/707#issuecomment-4121907946 */

    constexpr float HUE = 0.0f;               // [0,1] range.
    constexpr float SATURATION = 1.0f;        // [0,6] range.
    constexpr float SATURATION_ACCENT = 1.0f; // [0,6] range.
    constexpr float ALPHA = 0.99f;
    constexpr float BORDER_SIZE = 0.0f;

    style.FrameBorderSize = BORDER_SIZE;
    style.ImageBorderSize = BORDER_SIZE;
    style.TabBorderSize = BORDER_SIZE;
    style.TabBarBorderSize = 3.0f;
    style.WindowRounding = 4.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;

    ImVec4 TextColor{1.000f, 1.000f, 1.000f, 1.000f};
    ImVec4 TextDimmedColor{0.357f, 0.482f, 0.549f, 1.000f};
    ImVec4 BackroundColor{0.110f, 0.149f, 0.169f, 1.000f};
    ImVec4 BackroundChildColor{0.090f, 0.122f, 0.141f, 1.000f};
    ImVec4 BackroundDimmedColor{0.000f, 0.000f, 0.000f, 0.600f};
    ImVec4 TitleColor{0.078f, 0.102f, 0.122f, 1.000f};
    ImVec4 HeaderColor{0.184f, 0.247f, 0.286f, 1.000f};
    ImVec4 Accent1Color{0.000f, 0.490f, 1.000f, 1.000f};
    ImVec4 Accent2Color{0.000f, 0.412f, 0.824f, 1.000f};
    ImVec4 Accent1AlternativeColor{0.302f, 0.408f, 0.475f, 1.000f};
    ImVec4 Accent2AlternativeColor{0.251f, 0.337f, 0.392f, 1.000f};
    ImVec4 TransparentColor{0.0f, 0.0f, 0.0f, 0.0f};

    TextColor = adjust_hue_saturation(TextColor, HUE, SATURATION);
    TextDimmedColor = adjust_hue_saturation(TextDimmedColor, HUE, SATURATION);
    BackroundColor = adjust_hue_saturation(BackroundColor, HUE, SATURATION);
    BackroundChildColor = adjust_hue_saturation(BackroundChildColor, HUE, SATURATION);
    BackroundDimmedColor = adjust_hue_saturation(BackroundDimmedColor, HUE, SATURATION);
    TitleColor = adjust_hue_saturation(TitleColor, HUE, SATURATION);
    HeaderColor = adjust_hue_saturation(HeaderColor, HUE, SATURATION);
    Accent1Color = adjust_hue_saturation(Accent1Color, HUE, SATURATION_ACCENT);
    Accent2Color = adjust_hue_saturation(Accent2Color, HUE, SATURATION_ACCENT);
    Accent1AlternativeColor = adjust_hue_saturation(Accent1AlternativeColor, HUE, SATURATION_ACCENT);
    Accent2AlternativeColor = adjust_hue_saturation(Accent2AlternativeColor, HUE, SATURATION_ACCENT);

    ImVec4 BackroundTransparentColor = BackroundColor;
    BackroundTransparentColor.w *= ALPHA;

    ImVec4 BackroundChildTransparentColor = BackroundChildColor;
    BackroundChildTransparentColor.w *= ALPHA;

    ImVec4 TitleTransparentColor = TitleColor;
    TitleTransparentColor.w *= ALPHA;

    ImVec4 HeaderTransparentColor = HeaderColor;
    HeaderTransparentColor.w *= ALPHA;

    style.Colors[ImGuiCol_Text] = TextColor;
    style.Colors[ImGuiCol_TextDisabled] = TextDimmedColor;
    style.Colors[ImGuiCol_WindowBg] = BackroundTransparentColor;
    style.Colors[ImGuiCol_ChildBg] = BackroundChildTransparentColor;
    style.Colors[ImGuiCol_PopupBg] = BackroundChildTransparentColor;
    style.Colors[ImGuiCol_Border] = TitleColor;
    style.Colors[ImGuiCol_BorderShadow] = TransparentColor;
    style.Colors[ImGuiCol_FrameBg] = HeaderTransparentColor;
    style.Colors[ImGuiCol_FrameBgHovered] = Accent1AlternativeColor;
    style.Colors[ImGuiCol_FrameBgActive] = Accent2AlternativeColor;
    style.Colors[ImGuiCol_TitleBg] = HeaderTransparentColor;
    style.Colors[ImGuiCol_TitleBgActive] = TitleTransparentColor;
    style.Colors[ImGuiCol_TitleBgCollapsed] = HeaderTransparentColor;
    style.Colors[ImGuiCol_MenuBarBg] = BackroundChildTransparentColor;
    style.Colors[ImGuiCol_ScrollbarBg] = TitleTransparentColor;
    style.Colors[ImGuiCol_ScrollbarGrab] = HeaderColor;
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = Accent1AlternativeColor;
    style.Colors[ImGuiCol_ScrollbarGrabActive] = Accent2AlternativeColor;
    style.Colors[ImGuiCol_CheckMark] = Accent1Color;
    style.Colors[ImGuiCol_SliderGrab] = Accent1Color;
    style.Colors[ImGuiCol_SliderGrabActive] = Accent2Color;
    style.Colors[ImGuiCol_Button] = HeaderTransparentColor;
    style.Colors[ImGuiCol_ButtonHovered] = Accent1Color;
    style.Colors[ImGuiCol_ButtonActive] = Accent2Color;
    style.Colors[ImGuiCol_Header] = HeaderTransparentColor;
    style.Colors[ImGuiCol_HeaderHovered] = Accent1Color;
    style.Colors[ImGuiCol_HeaderActive] = Accent2Color;
    style.Colors[ImGuiCol_Separator] = HeaderColor;
    style.Colors[ImGuiCol_SeparatorHovered] = Accent1Color;
    style.Colors[ImGuiCol_SeparatorActive] = Accent2Color;
    style.Colors[ImGuiCol_ResizeGrip] = HeaderColor;
    style.Colors[ImGuiCol_ResizeGripHovered] = Accent1Color;
    style.Colors[ImGuiCol_ResizeGripActive] = Accent2Color;
    style.Colors[ImGuiCol_InputTextCursor] = TextColor;
    style.Colors[ImGuiCol_TabHovered] = Accent1Color;
    style.Colors[ImGuiCol_Tab] = TransparentColor;
    style.Colors[ImGuiCol_TabSelected] = HeaderTransparentColor;
    style.Colors[ImGuiCol_TabSelectedOverline] = TransparentColor;
    style.Colors[ImGuiCol_TabDimmed] = TransparentColor;
    style.Colors[ImGuiCol_TabDimmedSelected] = BackroundChildTransparentColor;
    style.Colors[ImGuiCol_TabDimmedSelectedOverline] = TransparentColor;
    style.Colors[ImGuiCol_DockingPreview] = HeaderColor;
    style.Colors[ImGuiCol_DockingEmptyBg] = BackroundTransparentColor;
    style.Colors[ImGuiCol_PlotLines] = TextDimmedColor;
    style.Colors[ImGuiCol_PlotLinesHovered] = Accent1Color;
    style.Colors[ImGuiCol_PlotHistogram] = TextDimmedColor;
    style.Colors[ImGuiCol_PlotHistogramHovered] = Accent1Color;
    style.Colors[ImGuiCol_TableHeaderBg] = BackroundChildTransparentColor;
    style.Colors[ImGuiCol_TableBorderStrong] = TitleColor;
    style.Colors[ImGuiCol_TableBorderLight] = HeaderColor;
    style.Colors[ImGuiCol_TableRowBg] = BackroundTransparentColor;
    style.Colors[ImGuiCol_TableRowBgAlt] = BackroundChildTransparentColor;
    style.Colors[ImGuiCol_TextLink] = Accent1Color;
    style.Colors[ImGuiCol_TextSelectedBg] = Accent1Color;
    style.Colors[ImGuiCol_TreeLines] = TextDimmedColor;
    style.Colors[ImGuiCol_DragDropTarget] = Accent1Color;
    style.Colors[ImGuiCol_DragDropTargetBg] = TransparentColor;
    style.Colors[ImGuiCol_UnsavedMarker] = Accent1Color;
    style.Colors[ImGuiCol_NavCursor] = Accent1Color;
    style.Colors[ImGuiCol_NavWindowingHighlight] = Accent1Color;
    style.Colors[ImGuiCol_NavWindowingDimBg] = BackroundDimmedColor;
    style.Colors[ImGuiCol_ModalWindowDimBg] = BackroundDimmedColor;
}
