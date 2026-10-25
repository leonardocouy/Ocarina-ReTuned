// SPDX-License-Identifier: GPL-2.0-or-later
#include "../tools/cheats/challenges.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>

struct Memory {
    std::map<uint32_t, uint8_t> bytes;
    template <class T> bool Read(uint32_t address, T* value) {
        if (address > UINT32_MAX - (sizeof(T) - 1)) return false;
        uint32_t result = 0;
        for (unsigned i = 0; i < sizeof(T); ++i) {
            auto found = bytes.find(address + i);
            if (found == bytes.end()) return false;
            result |= uint32_t(found->second) << (i * 8);
        }
        *value = static_cast<T>(result);
        return true;
    }
    bool Read8(uint32_t address, uint8_t* value) { return Read(address, value); }
    bool Read16(uint32_t address, uint16_t* value) { return Read(address, value); }
    bool Read32(uint32_t address, uint32_t* value) { return Read(address, value); }
    template <class T> void Put(uint32_t address, T value) {
        for (unsigned i = 0; i < sizeof(T); ++i) bytes[address + i] = uint32_t(value) >> (i * 8);
    }
    bool Write16(uint32_t address, uint16_t value) {
        uint16_t previous;
        if (!Read16(address, &previous)) return false;
        Put(address, value);
        return true;
    }
    bool Write32(uint32_t address, uint32_t value) {
        uint32_t previous;
        if (!Read32(address, &previous)) return false;
        Put(address, value);
        return true;
    }
};

constexpr uint32_t Global = 0x10000, Player = 0x20000, Hid = 0x10002000;

Memory Ready() {
    Memory memory;
    memory.Put<uint32_t>(Comfort::SaveContext + 0x14E4, 0);
    memory.Put<uint16_t>(Comfort::SaveContext + 0x44, 48);
    memory.Put<uint32_t>(0x5043E8, 0);
    memory.Put<uint8_t>(Global + 0x22A0, 0);
    memory.Put<uint8_t>(Global + 0x5C2D, 0);
    memory.Put<uint32_t>(Global + 0x20AC, Player);
    memory.Put<uint16_t>(Player, 0);
    memory.Put<uint8_t>(Player + 2, 2);
    memory.Put<uint32_t>(Player + 0x64, 0xBF800000);
    memory.Put<uint32_t>(Hid + 0x10, 7);
    memory.Put<uint32_t>(Hid + 0x28 + 7 * 0x10, 0x200);
    memory.Put<uint32_t>(Player + 0x221C, 0x40A00000);
    memory.Put<uint32_t>(Hid + 0x28 + 7 * 0x10, 0x400);
    return memory;
}

void RefusesSpeed(Memory memory, uint32_t global = Global) {
    const auto before = memory.bytes;
    assert(!Comfort::RunFast(memory, global));
    assert(memory.bytes == before);
}

int main() {
    auto memory = Ready(), expected = memory;
    expected.Put<uint32_t>(Player + 0x221C, 0x41A00000);
    assert(Comfort::RunFast(memory, Global));
    assert(memory.bytes == expected.bytes);
    memory.Put<uint32_t>(Hid + 0x28 + 7 * 0x10, 0x200);
    RefusesSpeed(memory);
    for (uint32_t speed : {0U, 0x80000000U, 0xBF800000U, 0x41A00000U,
                           0x41A80000U, 0x7FC00000U, 0x7F800000U, 0xFF800000U}) {
        memory = Ready(); memory.Put(Player + 0x221C, speed); RefusesSpeed(memory);
    }
    for (uint32_t pointer : {0U, UINT32_MAX - 0x100U, 0x40000U}) {
        memory = Ready(); memory.Put(Global + 0x20AC, pointer); RefusesSpeed(memory);
    }
    memory = Ready(); memory.Put<uint16_t>(Player, 1); RefusesSpeed(memory);
    memory = Ready(); memory.Put<uint8_t>(Player + 2, 3); RefusesSpeed(memory);
    memory = Ready(); memory.bytes.erase(Player + 0x221F); RefusesSpeed(memory);
    memory = Ready(); memory.Put<uint8_t>(Global + 0x22A0, 1); RefusesSpeed(memory);
    memory = Ready(); memory.Put<uint8_t>(Global + 0x5C2D, 1); RefusesSpeed(memory);
    memory = Ready(); memory.Put<uint32_t>(0x5043E8, 1); RefusesSpeed(memory);
    memory = Ready(); memory.Put<uint32_t>(Comfort::SaveContext + 0x14E4, 1); RefusesSpeed(memory);
    memory = Ready(); memory.Put<uint16_t>(Comfort::SaveContext + 0x44, 0); RefusesSpeed(memory);
    for (uint32_t index : {8U, UINT32_MAX}) {
        memory = Ready(); memory.Put(Hid + 0x10, index); RefusesSpeed(memory);
    }
    memory = Ready(); memory.bytes.erase(Hid + 0x28 + 7 * 0x10); RefusesSpeed(memory);
    RefusesSpeed(Ready(), 0); RefusesSpeed(Ready(), UINT32_MAX);

    constexpr uint32_t timer = Comfort::SaveContext + 0x1564;
    constexpr uint32_t state = Comfort::SaveContext + 0x1562;
    for (auto [remaining, held] : {std::pair{1, 120}, {119, 120}, {120, 120},
                                 {121, 121}, {122, 180}, {179, 180}, {180, 180},
                                 {181, 181}, {182, 240}, {239, 240}, {240, 240},
                                 {241, 240}, {32767, 240}}) {
        memory = Ready();
        memory.Put<uint16_t>(state, 4);
        memory.Put<uint16_t>(timer, remaining);
        expected = memory;
        expected.Put<uint16_t>(timer, held);
        assert(Comfort::FreezeTimers(memory) == (held != remaining));
        assert(memory.bytes == expected.bytes);
    }
    for (uint16_t inactive : {uint16_t(0), uint16_t(1), uint16_t(3), uint16_t(5), uint16_t(0xFFFF)}) {
        memory = Ready(); memory.Put(state, inactive); memory.Put<uint16_t>(timer, 119);
        const auto before = memory.bytes;
        assert(!Comfort::FreezeTimers(memory)); assert(memory.bytes == before);
    }
    for (uint16_t expired : {uint16_t(0), uint16_t(0x8000), uint16_t(0xFFFF)}) {
        memory = Ready(); memory.Put<uint16_t>(state, 4); memory.Put(timer, expired);
        const auto before = memory.bytes;
        assert(!Comfort::FreezeTimers(memory)); assert(memory.bytes == before);
    }
    for (uint32_t missing : {state, state + 1, timer, timer + 1}) {
        memory = Ready(); memory.Put<uint16_t>(state, 4); memory.Put<uint16_t>(timer, 119);
        memory.bytes.erase(missing);
        const auto before = memory.bytes;
        assert(!Comfort::FreezeTimers(memory)); assert(memory.bytes == before);
    }
    std::cout << "Passed: active secondary timer bands, untouched inactive timers, and X-only dynamic player speed with gameplay guards.\n";
}
