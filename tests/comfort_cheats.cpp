// SPDX-License-Identifier: GPL-2.0-or-later
#include "../tools/cheats/comfort.h"
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>

struct Memory {
    std::array<uint8_t, 0x600000> bytes{};
    unsigned writes = 0;
    bool Read8(uint32_t address, uint8_t* value) {
        if (address >= bytes.size()) return false;
        *value = bytes[address]; return true;
    }
    bool Read32(uint32_t address, uint32_t* value) {
        if (address > bytes.size() - 4) return false;
        std::memcpy(value, bytes.data() + address, 4); return true;
    }
    bool Write8(uint32_t address, uint8_t value) {
        if (address >= bytes.size()) return false;
        bytes[address] = value; ++writes; return true;
    }
    bool Write16(uint32_t address, uint16_t value) {
        if (address > bytes.size() - 2) return false;
        std::memcpy(bytes.data() + address, &value, 2); ++writes; return true;
    }
    bool Write32(uint32_t address, uint32_t value) {
        if (address > bytes.size() - 4) return false;
        std::memcpy(bytes.data() + address, &value, 4); ++writes; return true;
    }
    uint32_t Word(uint32_t address) { uint32_t value; assert(Read32(address, &value)); return value; }
};

int main() {
    static Memory memory;
    constexpr uint32_t object = 0x100, buffer = 0x1000, pointers = 0x6000, text = 0x9000;
    memory.Write32(object + 0xDC, buffer);
    memory.Write32(object + 0xD4, pointers);
    assert(Comfort::BeginInstantText(memory, object, text, 0));
    assert(memory.Word(object + 0xD8) == 1 && memory.Word(object + 0xE0) == 8);
    assert(memory.Word(buffer) == 0x4ED174 && memory.bytes[buffer + 4] == 6);
    assert(memory.Word(pointers) == buffer);
    auto previous = memory.writes;
    assert(!Comfort::BeginInstantText(memory, object, text, 1));
    assert(memory.writes == previous);
    memory.Write8(text - 2, 0x7F); memory.Write8(text - 1, 1);
    assert(Comfort::BeginInstantText(memory, object, text, 2));
    assert(memory.Word(object + 0xD8) == 2 && memory.Word(pointers + 4) == buffer + 8);
    memory.Write32(object + 0xD8, 0x200); previous = memory.writes;
    assert(!Comfort::BeginInstantText(memory, object, text, 0) && memory.writes == previous);
    memory.Write32(object + 0xD8, 0); memory.Write32(object + 0xE0, 0x3FF9); previous = memory.writes;
    assert(!Comfort::BeginInstantText(memory, object, text, 0) && memory.writes == previous);
    memory.Write32(object + 0xE0, 0); memory.Write32(object + 0xDC, UINT32_MAX); previous = memory.writes;
    assert(!Comfort::BeginInstantText(memory, object, text, 0) && memory.writes == previous);
    assert(!Comfort::BeginInstantText(memory, UINT32_MAX, text, 0) && memory.writes == previous);
    for (unsigned code = 0; code < 0x30; ++code) {
        memory.Write8(0xA004, code);
        const bool changed = Comfort::MakeControlSkippable(memory, 0xA000);
        assert(changed == (code == 5 || code == 0x19));
        assert(memory.bytes[0xA004] == (changed ? 4 : code));
    }
    assert(!Comfort::MakeControlSkippable(memory, UINT32_MAX));
    memory.Write16(Comfort::Rupees, 12); memory.Write16(Comfort::PendingRupees, 0xFFFF);
    memory.Write8(Comfort::Rupees - 1, 0xAB); memory.Write8(Comfort::Rupees + 2, 0xCD);
    assert(Comfort::RefillWallet(memory));
    assert((memory.Word(Comfort::Rupees) & 0xFFFF) == 999);
    assert((memory.Word(Comfort::PendingRupees) & 0xFFFF) == 0);
    assert(memory.bytes[Comfort::Rupees - 1] == 0xAB && memory.bytes[Comfort::Rupees + 2] == 0xCD);
    std::cout << "Passed: instant text, page breaks, capacity/overflow guards, preserved choices, wallet refill.\n";
}
