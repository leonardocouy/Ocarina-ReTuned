// SPDX-License-Identifier: GPL-2.0-or-later
#include "../tools/cheats/warps.h"

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
    template <class T> bool Write(uint32_t address, T value) {
        T previous;
        if (!Read(address, &previous)) return false;
        Put(address, value);
        return true;
    }
    bool Write8(uint32_t address, uint8_t value) { return Write(address, value); }
    bool Write16(uint32_t address, uint16_t value) { return Write(address, value); }
    bool Write32(uint32_t address, uint32_t value) { return Write(address, value); }
};

constexpr uint32_t Global = 0x10000, Player = 0x20000, Save = Comfort::SaveContext;

Memory Ready(uint8_t age = 0) {
    Memory memory;
    memory.Put<uint32_t>(Save, 0xDB);
    memory.Put<uint32_t>(Save + 4, age);
    memory.Put<uint32_t>(Save + 8, 0xFFF1);
    memory.Put<uint16_t>(Save + 0x44, 48);
    memory.Put<uint32_t>(Save + 0x14E4, 0);
    memory.Put<uint16_t>(Save + 0x15A0, 0xFFF2);
    memory.Put<uint32_t>(0x5043E8, 0);
    memory.Put<uint8_t>(Global + 0x22A0, 0);
    memory.Put<uint8_t>(Global + 0x5C2D, 0);
    memory.Put<uint32_t>(Global + 0x20AC, Player);
    memory.Put<uint16_t>(Player, 0);
    memory.Put<uint8_t>(Player + 2, 2);
    memory.Put<uint32_t>(Player + 0x64, 0xBF800000);
    memory.Put<uint8_t>(Global + 0x5C00, age);
    memory.Put<uint16_t>(Global + 0x5C32, 0xFFFF);
    memory.Put<uint8_t>(Global + 0x5C76, 0xAA);
    return memory;
}

void Refuses(Memory memory, uint32_t index = 1, uint32_t global = Global) {
    const auto before = memory.bytes;
    assert(!Comfort::Warp(memory, global, index));
    assert(memory.bytes == before);
}

void Lands(Memory memory, uint32_t index, uint16_t entrance) {
    auto expected = memory;
    expected.Put<uint32_t>(Save + 8, 0);
    expected.Put<uint16_t>(Save + 0x15A0, 0xFFEF);
    expected.Put<uint16_t>(Global + 0x5C32, entrance);
    expected.Put<uint8_t>(Global + 0x5C76, 3);
    expected.Put<uint8_t>(Global + 0x5C2D, 0x14);
    assert(Comfort::Warp(memory, Global, index));
    assert(memory.bytes == expected.bytes);
}

int main() {
    for (uint8_t age : {0, 1}) {
        for (uint32_t index = 1; index < RetunedComfort::Destinations.size(); ++index)
            Lands(Ready(age), index, RetunedComfort::Destinations[index].Entrance);
        Lands(Ready(age), 0, 0xDB);
        auto memory = Ready(age);
        memory.Put<uint8_t>(Global + 0x5C00, 1 - age);
        Lands(memory, 1, 0xEE);
    }
    for (uint32_t index : {uint32_t(RetunedComfort::Destinations.size()), UINT32_MAX}) Refuses(Ready(), index);
    for (uint32_t global : {0U, UINT32_MAX, UINT32_MAX - 0x5C75}) Refuses(Ready(), 1, global);
    for (uint16_t pending : {0U, 0xDBU, 0xFFFEU}) {
        auto memory = Ready(); memory.Put<uint16_t>(Global + 0x5C32, pending); Refuses(memory);
    }
    for (uint8_t age : {2, 0xFF}) {
        auto memory = Ready(); memory.Put<uint32_t>(Save + 4, age); Refuses(memory);
        memory = Ready(); memory.Put<uint8_t>(Global + 0x5C00, age); Refuses(memory);
    }
    for (uint32_t bad : {0x611U, 0x614U, 0xFFFFU, UINT32_MAX}) {
        auto memory = Ready(); memory.Put<uint32_t>(Save, bad); Refuses(memory, 0);
    }
    auto edge = Ready(); edge.Put<uint32_t>(Save, 0x610); Lands(edge, 0, 0x610);
    auto pending = Ready(); pending.Put<uint8_t>(Global + 0x5C2D, 0x14); Refuses(pending);
    auto paused = Ready(); paused.Put<uint32_t>(0x5043E8, 1); Refuses(paused);
    auto cutscene = Ready(); cutscene.Put<uint8_t>(Global + 0x22A0, 1); Refuses(cutscene);
    const auto complete = Ready();
    for (const auto& [address, ignored] : complete.bytes) {
        auto incomplete = complete; incomplete.bytes.erase(address);
        Refuses(incomplete, 0);
    }
    std::cout << "Warp behavior checks passed\n";
}
