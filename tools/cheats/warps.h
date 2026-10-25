// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "destinations.h"
#include "levitation.h"

namespace Comfort {
// Adapted from OcarinaCTRComposer (MIT), copyright 2026 samabr (samaBR85).
// Use the game's entrance transition instead of moving Link through unloaded rooms.
template <class Memory>
bool Warp(Memory& memory, uint32_t global, uint32_t index) {
    if (index >= RetunedComfort::Destinations.size() || !global || global > UINT32_MAX - 0x5C76) return false;
    uint32_t player = 0, age = 0, cutscene = 0;
    uint16_t nextEntrance = 0, nextCutscene = 0;
    uint8_t ageOnLoad = 0, fade = 0;
    if (!ReadPlayer(memory, global, &player) ||
        !memory.Read16(global + 0x5C32, &nextEntrance) || nextEntrance != 0xFFFF ||
        !memory.Read32(SaveContext + 4, &age) || age > 1 ||
        !memory.Read8(global + 0x5C00, &ageOnLoad) || ageOnLoad > 1 ||
        !memory.Read32(SaveContext + 8, &cutscene) ||
        !memory.Read16(SaveContext + 0x15A0, &nextCutscene) ||
        !memory.Read8(global + 0x5C76, &fade)) return false;

    uint16_t entrance = RetunedComfort::Destinations[index].Entrance;
    if (entrance == 0xFFFF) {
        uint32_t current = 0;
        // The original table has 0x614 entries. Each base needs four age/time variants.
        if (!memory.Read32(SaveContext, &current) || current > 0x610) return false;
        entrance = static_cast<uint16_t>(current);
    }
    return memory.Write32(SaveContext + 8, 0) &&
           memory.Write16(SaveContext + 0x15A0, 0xFFEF) &&
           memory.Write16(global + 0x5C32, entrance) &&
           memory.Write8(global + 0x5C76, 3) &&
           memory.Write8(global + 0x5C2D, 0x14);
}
}
