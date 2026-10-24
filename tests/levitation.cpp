// SPDX-License-Identifier: GPL-2.0-or-later
#include "../tools/cheats/levitation.h"

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
    return memory;
}

void Refuses(Memory memory, uint32_t global = Global) {
    const auto before = memory.bytes;
    assert(!Comfort::Levitate(memory, global));
    assert(memory.bytes == before);
}

int main() {
    auto memory = Ready();
    auto expected = memory;
    expected.Put<uint32_t>(Player + 0x64, 0x40CB0000);
    assert(Comfort::Levitate(memory, Global));
    assert(memory.bytes == expected.bytes);
    memory.Put<uint32_t>(Hid + 0x28 + 7 * 0x10, 0x20);
    Refuses(memory);
    for (uint32_t pointer : {0U, UINT32_MAX, 0x40000U}) {
        memory = Ready(); memory.Put<uint32_t>(Global + 0x20AC, pointer); Refuses(memory);
    }
    memory = Ready(); memory.Put<uint16_t>(Player, 1); Refuses(memory);
    memory = Ready(); memory.Put<uint8_t>(Player + 2, 3); Refuses(memory);
    memory = Ready(); memory.bytes.erase(Player + 0x67); Refuses(memory);
    memory = Ready(); memory.Put<uint8_t>(Global + 0x22A0, 1); Refuses(memory);
    memory = Ready(); memory.Put<uint8_t>(Global + 0x5C2D, 1); Refuses(memory);
    memory = Ready(); memory.Put<uint32_t>(0x5043E8, 1); Refuses(memory);
    memory = Ready(); memory.Put<uint32_t>(Comfort::SaveContext + 0x14E4, 1); Refuses(memory);
    for (uint16_t health : {uint16_t(0), uint16_t(0xFFFF)}) {
        memory = Ready(); memory.Put<uint16_t>(Comfort::SaveContext + 0x44, health); Refuses(memory);
    }
    for (uint32_t index : {8U, UINT32_MAX}) {
        memory = Ready(); memory.Put<uint32_t>(Hid + 0x10, index); Refuses(memory);
    }
    memory = Ready(); memory.bytes.erase(Hid + 0x28 + 7 * 0x10); Refuses(memory);
    Refuses(Ready(), 0); Refuses(Ready(), UINT32_MAX);
    uint32_t player = 123;
    memory = Ready(); memory.bytes.erase(Global + 0x20AC);
    assert(!Comfort::ReadPlayer(memory, Global, &player) && player == 123);
    memory = Ready(); assert(!Comfort::ReadPlayer(memory, Global, nullptr));
    std::cout << "Passed: levitation changes only vertical velocity while L is held; gameplay and memory guards reject unsafe states.\n";
}
