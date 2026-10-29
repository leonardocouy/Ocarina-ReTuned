// SPDX-License-Identifier: GPL-2.0-or-later
#include "../tools/cheats/fishing.h"
#include <cassert>
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
};

constexpr uint32_t Global = 0x10000, Player = 0x20000;

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
    memory.Put<uint16_t>(Player + 0x90, 1);
    return memory;
}

void Bite(Memory& memory) {
    memory.Put<uint16_t>(Global + 0x104, 73);
    memory.Put<uint16_t>(Comfort::FishingState + 0x1E, 4);
    memory.Put<uint8_t>(Comfort::FishingState + 0x1A, 0);
    memory.Put<uint16_t>(Comfort::FishingState + 0x3C, 20);
    memory.Put<uint32_t>(Comfort::FishingState + 0xCC, 0x30000);
    memory.Put<uint16_t>(0x30000, 0xFE);
    memory.Put<uint32_t>(0x3013C, 0x1F9ACC);
    memory.Put<uint16_t>(0x301B0, 5);
}

int main() {
    auto memory = Ready();
    Bite(memory);
    const auto before = memory.bytes;
    assert(Comfort::FishingBite(memory, Global));
    assert(memory.bytes == before);
    for (const auto [address, value] : std::map<uint32_t, uint32_t>{
        {Global + 0x104, 72},
        {Comfort::FishingState + 0x1E, 3}, {Comfort::FishingState + 0x1A, 1},
        {Comfort::FishingState + 0x3C, 0}, {Comfort::FishingState + 0xCC, UINT32_MAX},
        {0x30000, 0x95}, {0x3013C, 0}, {0x301B0, 4},
        {0x5043E8, 1}, {Global + 0x22A0, 1}, {Global + 0x5C2D, 1},
        {Comfort::SaveContext + 0x44, 0}}) {
        memory = Ready(); Bite(memory);
        memory.Put<uint32_t>(address, value);
        assert(!Comfort::FishingBite(memory, Global));
    }
    memory = Ready(); Bite(memory);
    memory.bytes.erase(Comfort::FishingState + 0x3C);
    assert(!Comfort::FishingBite(memory, Global));
    assert(!Comfort::FishingBite(memory, UINT32_MAX));
    std::cout << "Passed: fishing cue follows the bite window, hides after hooking or expiry, and ignores inactive play.\n";
}
