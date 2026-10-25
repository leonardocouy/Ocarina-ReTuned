// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "levitation.h"
#include <bit>
#include <cmath>

namespace Comfort {
template <class Memory>
bool FreezeTimers(Memory& memory) {
    uint16_t state = 0, remaining = 0;
    if (!memory.Read16(SaveContext + 0x1562, &state) || state != 4 ||
        !memory.Read16(SaveContext + 0x1564, &remaining) ||
        remaining == 0 || remaining > 0x7FFF) return false;
    // Match the original code's strict comparison boundaries for the secondary timer.
    uint16_t held = remaining;
    if (remaining < 121) held = 120;
    else if (remaining > 121 && remaining < 181) held = 180;
    else if (remaining > 181) held = 240;
    return held != remaining && memory.Write16(SaveContext + 0x1564, held);
}

template <class Memory>
bool RunFast(Memory& memory, uint32_t global) {
    uint32_t player = 0, buttons = 0, speedBits = 0;
    if (!ReadPlayer(memory, global, &player) || player > UINT32_MAX - 0x221F ||
        !ReadButtons(memory, &buttons) || !(buttons & 0x400) ||
        !memory.Read32(player + 0x221C, &speedBits)) return false;
    const float speed = std::bit_cast<float>(speedBits);
    if (!std::isfinite(speed) || speed <= 0 || speed >= 20) return false;
    return memory.Write32(player + 0x221C, 0x41A00000);
}
}
