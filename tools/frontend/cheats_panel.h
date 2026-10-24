// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "oot3d_native_direct_aot.h"
#include "fast/oot3d/graphics_settings_window.h"
#include "fast/oot3d/retuned_blue_style.h"
#include <imgui.h>

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
        for (size_t i = 0; i < RetunedComfort::Settings.size(); ++i) {
            const auto& setting = RetunedComfort::Settings[i];
            const auto cheat = static_cast<RetunedComfort::Cheat>(i);
            bool enabled = false;
            if (!cheats->Get(cheat, &enabled)) continue;
            if (Toggle(setting.Label, &enabled)) cheats->Set(cheat, enabled);
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            ImGui::TextWrapped("%s", setting.Help);
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }
        ImGui::SeparatorText("Time of day");
        ImGui::TextWrapped("Change time once. The normal day cycle continues afterward.");
        ImGui::Combo("Time", &mTime,
            "Choose time\0Sunrise\0Daylight\0Bright daylight\0Sunset\0Nightfall\0");
        ImGui::BeginDisabled(mTime == 0);
        if (ImGui::Button("Set time"))
            cheats->Request(RetunedComfort::Action::SetTime, RetunedComfort::Times[mTime - 1]);
        ImGui::EndDisabled();
        ImGui::SeparatorText("Bottle contents");
        ImGui::TextWrapped("Change an existing bottle once. Unequip bottles first. Changes apply when play resumes.");
        for (size_t i = 0; i < mBottles.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            ImGui::Text("Bottle %u", static_cast<unsigned>(i + 1));
            ImGui::Combo("Contents", &mBottles[i],
                "Unchanged\0Empty bottle\0Red potion\0Green potion\0Blue potion\0Fairy\0Fish\0Lon Lon milk\0Ruto's letter\0Blue fire\0Bug\0Big Poe\0Half milk\0Poe\0");
            ImGui::BeginDisabled(mBottles[i] == 0);
            if (ImGui::Button("Apply contents")) {
                cheats->Request(static_cast<RetunedComfort::Action>(
                    static_cast<size_t>(RetunedComfort::Action::Bottle1) + i), 0x13 + mBottles[i]);
                mBottles[i] = 0;
            }
            ImGui::EndDisabled();
            ImGui::PopID();
        }
        const char* status = cheats->LastError();
        if (status && *status) ImGui::TextWrapped("%s", status);
    }
  private:
    int mTime = 0;
    std::array<int, 4> mBottles{};
};
}
