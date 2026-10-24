// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>

namespace Comfort {
constexpr uint32_t SaveContext = 0x587958;
constexpr uint32_t Rupees = SaveContext + 0x48;
constexpr uint32_t PendingRupees = SaveContext + 0x5C;

template <class Memory>
bool RefillWallet(Memory& memory) {
    return memory.Write16(Rupees, 999) && memory.Write16(PendingRupees, 0);
}

template <class Memory>
bool MakeControlSkippable(Memory& memory, uint32_t control) {
    uint8_t value = 0;
    if (control > UINT32_MAX - 4 || !memory.Read8(control + 4, &value)) return false;
    if (value != 0x05 && value != 0x19) return false;
    return memory.Write8(control + 4, 0x04);
}

template <class Memory>
bool BeginInstantText(Memory& memory, uint32_t object, uint32_t nextChars, uint32_t index) {
    if (index != 0) {
        uint8_t prefix = 0, command = 0;
        if (index < 2 || nextChars < 2 ||
            !memory.Read8(nextChars - 2, &prefix) ||
            !memory.Read8(nextChars - 1, &command) || prefix != 0x7F || command != 1) return false;
    }
    uint32_t count = 0, used = 0, commands = 0, pointers = 0;
    if (!object || object > UINT32_MAX - 0xE4 ||
        !memory.Read32(object + 0xD8, &count) || count >= 0x200 ||
        !memory.Read32(object + 0xE0, &used) || used > 0x4000 - 8 ||
        !memory.Read32(object + 0xDC, &commands) || !commands || commands > UINT32_MAX - used - 8 ||
        !memory.Read32(object + 0xD4, &pointers) || !pointers || pointers > UINT32_MAX - count * 4) return false;
    const uint32_t command = commands + used;
    uint32_t previous = 0;
    uint8_t previousKind = 0;
    if (!memory.Read32(command, &previous) || !memory.Read8(command + 4, &previousKind) ||
        !memory.Read32(pointers + count * 4, &previous)) return false;
    // Reproduce Message_SetInstantText's command ABI from this game's binary.
    return memory.Write32(command, 0x4ED174) && memory.Write8(command + 4, 6) &&
        memory.Write32(pointers + count * 4, command) &&
        memory.Write32(object + 0xE0, used + 8) && memory.Write32(object + 0xD8, count + 1);
}
}
