// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "comfort.h"
#include <array>

namespace Comfort {
enum class Ammo { Bombs, Nuts, Seeds, Sticks, Arrows, Beans };

template <class Memory>
bool RefillAmmo(Memory& memory, Ammo type) {
    uint32_t slot = 0, shift = 0;
    uint8_t item = 0, baseCapacity = 0;
    switch (type) {
    case Ammo::Bombs: slot = 2; item = 2; shift = 3; baseCapacity = 20; break;
    case Ammo::Nuts: slot = 1; item = 1; shift = 20; baseCapacity = 20; break;
    case Ammo::Seeds: slot = 6; item = 6; shift = 14; baseCapacity = 30; break;
    case Ammo::Sticks: slot = 0; item = 0; shift = 17; baseCapacity = 10; break;
    case Ammo::Arrows: slot = 3; item = 3; shift = 0; baseCapacity = 30; break;
    case Ammo::Beans: slot = 14; item = 0x10; break;
    default: return false;
    }
    uint8_t owned = 0, previous = 0;
    if (!memory.Read8(SaveContext + 0x8C + slot, &owned) ||
        (owned != item && !(type == Ammo::Arrows && owned >= 0x38 && owned <= 0x3A)) ||
        !memory.Read8(SaveContext + 0xA6 + slot, &previous)) return false;
    uint8_t capacity = 10;
    if (type != Ammo::Beans) {
        uint32_t upgrades = 0;
        if (!memory.Read32(SaveContext + 0xB8, &upgrades)) return false;
        const uint32_t level = (upgrades >> shift) & 7;
        if (level == 0 || level > 3) return false;
        capacity = static_cast<uint8_t>(baseCapacity + 10 * (level - 1));
    }
    return memory.Write8(SaveContext + 0xA6 + slot, capacity);
}

template <class Memory>
bool SetTime(Memory& memory, uint16_t time) {
    switch (time) {
    case 0x4000: case 0x5800: case 0x7000: case 0xC000: case 0xD000: break;
    default: return false;
    }
    uint8_t previous = 0;
    uint32_t nightFlag = 0;
    for (const uint32_t offset : {0xCU, 0xDU, 0x15A8U, 0x15A9U}) {
        if (!memory.Read8(SaveContext + offset, &previous)) return false;
    }
    if (!memory.Read32(SaveContext + 0x10, &nightFlag)) return false;
    return memory.Write16(SaveContext + 0xC, time) &&
           memory.Write16(SaveContext + 0x15A8, time) &&
           memory.Write32(SaveContext + 0x10, time >= 0xC000 ? 1 : 0);
}

template <class Memory>
bool SetBottle(Memory& memory, uint32_t slot, uint8_t content, uint32_t player) {
    if (slot >= 4 || content < 0x14 || content > 0x20 ||
        !player || player > UINT32_MAX - 0x1AA) return false;
    uint8_t held = 0, owned = 0;
    if (!memory.Read8(player + 0x1AA, &held) || (held >= 0x14 && held <= 0x20) ||
        !memory.Read8(SaveContext + 0x9E + slot, &owned) || owned < 0x14 || owned > 0x20) return false;

    std::array<uint32_t, 13> addresses{};
    uint32_t count = 1;
    addresses[0] = SaveContext + 0x9E + slot;
    for (const uint32_t block : {0x54U, 0x60U, 0x80U}) {
        for (uint32_t button = 0; button < 4; ++button) {
            uint8_t equippedSlot = 0, previous = 0;
            if (!memory.Read8(SaveContext + block + 5 + button, &equippedSlot)) return false;
            if (equippedSlot == 0x12 + slot) {
                const uint32_t address = SaveContext + block + 1 + button;
                if (!memory.Read8(address, &previous)) return false;
                addresses[count++] = address;
            }
        }
    }
    for (uint32_t index = 0; index < count; ++index) {
        if (!memory.Write8(addresses[index], content)) return false;
    }
    return true;
}
}
