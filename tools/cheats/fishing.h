// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "levitation.h"

namespace Comfort {
inline constexpr uint32_t FishingState = 0x53142C;

template<class Memory>
bool FishingBite(Memory& memory, uint32_t global) {
    uint32_t player = 0, fish = 0, update = 0;
    uint16_t scene = 0, cast = 0, timer = 0, id = 0, state = 0;
    uint8_t hooked = 0;
    return ReadPlayer(memory, global, &player) &&
        memory.Read16(global + 0x104, &scene) && scene == 73 &&
        memory.Read16(FishingState + 0x1E, &cast) && cast == 4 &&
        memory.Read8(FishingState + 0x1A, &hooked) && hooked == 0 &&
        memory.Read16(FishingState + 0x3C, &timer) && timer > 0 &&
        memory.Read32(FishingState + 0xCC, &fish) && fish && fish <= UINT32_MAX - 0x1B1 &&
        memory.Read16(fish, &id) && id == 0xFE &&
        memory.Read32(fish + 0x13C, &update) && update != 0 &&
        memory.Read16(fish + 0x1B0, &state) && state == 5;
}
}
