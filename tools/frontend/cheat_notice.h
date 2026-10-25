// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "oot3d_native_direct_aot.h"
#include "ship/window/gui/Gui.h"
#include <imgui.h>
#include <algorithm>
#include <string>

namespace RetunedFrontend {
inline void DrawCheatNotice(Ship::Gui&) {
    static uint64_t serial = 0;
    static std::string text;
    static double until = 0;
    if (const auto* cheats = Oot3dNativeGame::ComfortCheats()) {
        if (const char* message = cheats->Notice(&serial)) {
            text = message;
            until = ImGui::GetTime() + 3.0;
        }
    }
    if (text.empty() || ImGui::GetTime() >= until) return;
    const float padding = ImGui::GetFontSize() * .6f;
    const float wrap = std::max(1.f, ImGui::GetIO().DisplaySize.x - padding * 4);
    const auto size = ImGui::CalcTextSize(text.c_str(), nullptr, false, wrap);
    const ImVec2 position{(ImGui::GetIO().DisplaySize.x - size.x) * .5f, ImGui::GetFontSize() * 2};
    auto* draw = ImGui::GetForegroundDrawList();
    draw->AddRectFilled({position.x - padding, position.y - padding},
        {position.x + size.x + padding, position.y + size.y + padding}, IM_COL32(20, 35, 60, 240), padding * .5f);
    draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(), position, IM_COL32(255, 255, 255, 255),
        text.c_str(), nullptr, wrap);
}
}
