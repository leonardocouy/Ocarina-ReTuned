// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "levitation.h"
#include <algorithm>
#include <optional>

namespace Comfort {
inline constexpr uint32_t FishingState = 0x53142C;
struct FishingStatus {
    float Risk;
    bool Hooked;
};

template<class Memory>
std::optional<FishingStatus> ReadFishing(Memory& memory, uint32_t global) {
    uint32_t player = 0, fish = 0, update = 0, timing = 0;
    uint16_t scene = 0, cast = 0, timer = 0, id = 0, state = 0, pullback = 0, fight = 0, divisor = 0;
    uint8_t hooked = 0, reelLock = 0;
    if (!ReadPlayer(memory, global, &player) ||
        !memory.Read16(global + 0x104, &scene) || scene != 73 ||
        !memory.Read16(FishingState + 0x1E, &cast) || cast != 4 ||
        !memory.Read8(FishingState + 0x1A, &hooked) || hooked > 1 ||
        !memory.Read16(FishingState + 0x3C, &timer) || (!hooked && !timer) ||
        !memory.Read32(FishingState + 0xCC, &fish) || !fish || fish > UINT32_MAX - 0x1B1 ||
        !memory.Read16(fish, &id) || id != 0xFE ||
        !memory.Read32(fish + 0x13C, &update) || !update ||
        !memory.Read16(fish + 0x1B0, &state) || state != 5) return {};
    if (!hooked) return FishingStatus{1.f, false};
    if (!memory.Read16(FishingState + 0x3A, &pullback) || pullback > 100 ||
        !memory.Read16(FishingState + 0x26, &fight) ||
        !memory.Read8(FishingState + 0xC, &reelLock) ||
        !memory.Read32(0x51B2F4, &timing) || !timing || timing > UINT32_MAX - 0x111 ||
        !memory.Read16(timing + 0x110, &divisor) || !divisor) return {};
    const float limit = 18000.f / divisor;
    const float deadlineRisk = std::clamp((fight - limit + 300.f / divisor) / (300.f / divisor), 0.f, 1.f);
    const float slackRisk = 1.f - pullback / 100.f;
    const float risk = reelLock && fight > 150.f / divisor ? 1.f : std::max(slackRisk, deadlineRisk);
    return FishingStatus{risk, true};
}

template<class Memory>
void SetSinkingLure(Memory& memory, uint32_t global, bool enabled) {
    uint32_t player = 0;
    uint16_t scene = 0, playing = 0, cast = 0;
    uint8_t lure = 0;
    if (!ReadPlayer(memory, global, &player) ||
        !memory.Read16(global + 0x104, &scene) || scene != 73 ||
        !memory.Read16(FishingState + 0x30, &playing) || playing != 2 ||
        !memory.Read16(FishingState + 0x1E, &cast) || cast != 0 ||
        !memory.Read8(FishingState + 0x16, &lure) || lure > 2) return;
    const uint8_t desired = enabled ? 2 : 0;
    if (lure != desired) memory.Write8(FishingState + 0x16, desired);
}

class FishingFeedback {
  public:
    bool Update(const std::optional<FishingStatus>& status, uint64_t milliseconds) {
        if (!status) { active = hooked = false; return false; }
        const bool changed = !active || hooked != status->Hooked;
        active = true;
        hooked = status->Hooked;
        const auto interval = static_cast<uint64_t>(1000.f / (1.f + status->Risk * 2.f));
        if (!changed && milliseconds - lastPulse < interval) return false;
        lastPulse = milliseconds;
        return true;
    }
  private:
    bool active = false, hooked = false;
    uint64_t lastPulse = 0;
};
}
