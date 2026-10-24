// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "oot3d_native_direct_aot.h"
#include "fast/oot3d/graphics_settings_window.h"
#include "fast/oot3d/retuned_blue_style.h"
#include <imgui.h>
#include <string>

namespace RetunedFrontend {
class CheatsPanel final : public Fast::Oot3d::GraphicsSettingsPanelTab {
  public:
    const char* Label() const noexcept override { return "Cheats"; }
    void Draw() override {
        const auto* cheats = Oot3dNativeGame::ComfortCheats();
        if (!cheats) {
            ImGui::TextWrapped("Cheats are unavailable for this game installation.");
            return;
        }
        auto toggle = [&](RetunedComfort::Cheat cheat, const char* label, const char* help) {
            bool enabled = false;
            if (!cheats->Get(cheat, &enabled)) return;
            if (Toggle(label, &enabled)) {
                if (cheats->Set(cheat, enabled)) mError.clear();
                else mError = cheats->LastError();
            }
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            ImGui::TextWrapped("%s", help);
            ImGui::PopStyleColor();
            ImGui::Spacing();
        };
        toggle(RetunedComfort::Cheat::InfiniteRupees, "Infinite rupees", "Keep the balance at 999.");
        ImGui::Separator();
        ImGui::Spacing();
        toggle(RetunedComfort::Cheat::InstantDialogue, "Instant dialogue",
            "Show each page immediately. Press A to continue.");
        if (!mError.empty()) ImGui::TextWrapped("%s", mError.c_str());
    }
  private:
    std::string mError;
};
}
