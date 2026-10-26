// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "oot3d_native_direct_aot.h"
#include "ship/window/gui/Gui.h"
#include "fast/Fast3dGui.h"
#include "oot3d_native_ui_lifecycle_bridge.h"
#include "oot3d_native_ui_texture_provider.h"
#include <cstdlib>
#include <cstring>
#include <imgui.h>
#include <algorithm>
#include <string>
#include <cmath>

namespace RetunedFrontend {
inline ImTextureID SkulltulaIcon(Ship::Gui& gui,
    const Oot3dNativeGame::Oot3dNativeUiLifecycleBridge& bridge,
    const Oot3dNativeGame::Oot3dNativeA32UiTextureProvider* provider) {
    auto* fast = dynamic_cast<Fast::Fast3dGui*>(&gui);
    if (!fast || !provider) return 0;
    constexpr auto name = "retuned-skulltula-token";
    if (fast->HasTextureByName(name)) return fast->GetTextureByName(name);
    const auto identity = bridge.NativePauseSharedTextureIdentity(oot3d::ui::UiPauseSharedTextureSlot::ItemIcons);
    oot3d::ui::UiTexturePixels pixels;
    if (!identity || !provider->Resolve(*identity, pixels) || pixels.provisional ||
        !pixels.width || !pixels.height || pixels.rgba8.size() != std::size_t(pixels.width) * pixels.height * 4) return 0;
    auto texture = std::make_shared<Ship::GuiTexture>();
    texture->Data = static_cast<uint8_t*>(std::malloc(pixels.rgba8.size()));
    if (!texture->Data) return 0;
    std::memcpy(texture->Data, pixels.rgba8.data(), pixels.rgba8.size());
    texture->DataSize = pixels.rgba8.size();
    texture->Metadata.Width = pixels.width;
    texture->Metadata.Height = pixels.height;
    fast->LoadTextureFromResource(name, texture);
    return fast->GetTextureByName(name);
}
inline void DrawSkulltulaIndicator(Ship::Gui& gui,
    const Oot3dNativeGame::Oot3dNativeUiLifecycleBridge& bridge,
    const Oot3dNativeGame::Oot3dNativeA32UiTextureProvider* provider) {
    if (gui.GetAnyGuiWindowVisible() || gui.GetMenuOrMenubarVisible()) return;
    const auto* cheats = Oot3dNativeGame::ComfortCheats();
    float proximity = 0;
    bool token = false;
    if (!cheats || !cheats->Skulltula(&proximity, &token)) return;
    const char* text = token ? "Gold Skulltula token nearby"
        : proximity > .7f ? "Gold Skulltula very close" : "Gold Skulltula nearby";
    const float font = ImGui::GetFontSize();
    const float padding = font * .6f;
    const auto size = ImGui::CalcTextSize(text);
    const auto icon = SkulltulaIcon(gui, bridge, provider);
    const float iconSize = font * 2;
    const float iconSpace = icon ? iconSize + padding : 0;
    const float height = std::max(size.y, icon ? iconSize : 0.f);
    const auto display = ImGui::GetIO().DisplaySize;
    const ImVec2 position{(display.x - size.x - iconSpace) * .5f, display.y - font * 4};
    const double frequency = 1.0 + std::clamp(proximity, 0.f, 1.f);
    static double phase = 0;
    phase = std::fmod(phase + std::clamp(ImGui::GetIO().DeltaTime, 0.f, .1f) * frequency * 6.283185307, 6.283185307);
    const int alpha = static_cast<int>(170 + 85 * (.5 + .5 * std::sin(phase)));
    auto* draw = ImGui::GetForegroundDrawList();
    const ImVec2 from{position.x - padding, position.y - padding};
    const ImVec2 to{position.x + size.x + iconSpace + padding, position.y + height + padding};
    draw->AddRectFilled(from, to, IM_COL32(20, 25, 35, 220), padding * .5f);
    draw->AddRect(from, to, IM_COL32(255, 205, 70, alpha), padding * .5f);
    if (icon) {
        // The native 512px item atlas places ITEM_SKULL_TOKEN at (210,378), size 42px.
        draw->AddImage(icon, position, {position.x + iconSize, position.y + iconSize},
            {210.f / 512, 378.f / 512}, {252.f / 512, 420.f / 512}, IM_COL32(255, 255, 255, alpha));
    }
    draw->AddText({position.x + iconSpace, position.y + (height - size.y) * .5f},
        IM_COL32(255, 220, 100, alpha), text);
}

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
