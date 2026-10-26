// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include "destinations.h"

namespace RetunedComfort {
enum class Cheat : uint32_t {
    InfiniteRupees, InstantDialogue, Levitate, InfiniteBombs, InfiniteNuts,
    InfiniteSeeds, InfiniteSticks, InfiniteArrows, InfiniteBeans, FreezeTimers, RunFast, SkulltulaDetector, Count
};
struct CheatSetting {
    const char* Key;
    const char* Label;
    const char* Help;
};
inline constexpr std::array<CheatSetting, static_cast<std::size_t>(Cheat::Count)> Settings{{
    {"infinite_rupees", "Infinite rupees", "Keep the balance at 999."},
    {"instant_dialogue", "Instant dialogue", "Show each page immediately. Press A to continue."},
    {"levitate", "Levitate while holding L", "Hold L to rise. Hold L + R and press D-pad Up to toggle this cheat."},
    {"infinite_bombs", "Infinite bombs", "Refill your existing bomb bag."},
    {"infinite_nuts", "Infinite Deku nuts", "Refill nuts up to your current capacity."},
    {"infinite_seeds", "Infinite Deku seeds", "Refill seeds for your existing slingshot."},
    {"infinite_sticks", "Infinite Deku sticks", "Refill sticks up to your current capacity."},
    {"infinite_arrows", "Infinite arrows", "Refill arrows for your existing bow."},
    {"infinite_beans", "Infinite magic beans", "Keep ten beans if you already own them."},
    {"freeze_timers", "Freeze challenge timer", "Keep the secondary challenge or trade timer from running out."},
    {"run_fast", "Run fast while holding X", "Hold the game X button to run faster."},
    {"skulltula_detector", "Gold Skulltula detector", "Pulse an indicator near a loaded Gold Skulltula or an uncollected token."}
}};
enum class Action : uint32_t { SetTime, Bottle1, Bottle2, Bottle3, Bottle4, Teleport, Count };
inline constexpr std::array<uint16_t, 5> Times{0x4000, 0x5800, 0x7000, 0xC000, 0xD000};
struct Api {
    uint32_t Version;
    uint32_t Size;
    bool (*Get)(Cheat, bool*) noexcept;
    bool (*Set)(Cheat, bool) noexcept;
    bool (*Request)(Action, uint32_t) noexcept;
    const char* (*LastError)() noexcept;
    const char* (*Notice)(uint64_t*) noexcept;
    bool (*Skulltula)(float* proximity, bool* token) noexcept;
};
}
