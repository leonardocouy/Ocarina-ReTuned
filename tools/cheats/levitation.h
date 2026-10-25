// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "comfort.h"

namespace Comfort {
template <class Memory>
bool ReadPlayer(Memory& memory, uint32_t global, uint32_t* out) {
    if (!out || !global || global > UINT32_MAX - 0x5C2D) return false;
    uint32_t gameMode = 0, pause = 0, player = 0, velocity = 0;
    uint16_t health = 0, actorId = 0;
    uint8_t cutscene = 0, transition = 0, category = 0;
    if (!memory.Read32(SaveContext + 0x14E4, &gameMode) || gameMode != 0 ||
        !memory.Read16(SaveContext + 0x44, &health) || health == 0 || health > 0x7FFF ||
        !memory.Read32(0x5043E8, &pause) || pause != 0 ||
        !memory.Read8(global + 0x22A0, &cutscene) || cutscene != 0 ||
        !memory.Read8(global + 0x5C2D, &transition) || transition != 0 ||
        !memory.Read32(global + 0x20AC, &player) || !player || player > UINT32_MAX - 0x67 ||
        !memory.Read16(player, &actorId) || actorId != 0 ||
        !memory.Read8(player + 2, &category) || category != 2 ||
        !memory.Read32(player + 0x64, &velocity)) return false;
    *out = player;
    return true;
}

template <class Memory>
bool ReadButtons(Memory& memory, uint32_t* buttons) {
    uint32_t index = 0;
    constexpr uint32_t hid = 0x10002000;
    return buttons && memory.Read32(hid + 0x10, &index) && index < 8 &&
        memory.Read32(hid + 0x28 + index * 0x10, buttons);
}

template <class Memory>
bool Levitate(Memory& memory, uint32_t global) {
    uint32_t player = 0, buttons = 0;
    if (!ReadPlayer(memory, global, &player) || !ReadButtons(memory, &buttons) || !(buttons & 0x200)) return false;
    return memory.Write32(player + 0x64, 0x40CB0000);
}
}
