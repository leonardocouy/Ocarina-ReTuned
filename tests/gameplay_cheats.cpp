// SPDX-License-Identifier: GPL-2.0-or-later
#include "../tools/cheats/gameplay.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

struct Memory {
    std::vector<uint8_t> bytes = std::vector<uint8_t>(0x600000, 0xFF);
    uint32_t unavailable = UINT32_MAX;
    bool Read8(uint32_t address, uint8_t* value) {
        if (address >= bytes.size() || address == unavailable) return false;
        *value = bytes[address];
        return true;
    }
    bool Read32(uint32_t address, uint32_t* value) {
        if (address > bytes.size() - 4 ||
            (unavailable >= address && unavailable - address < 4)) return false;
        std::memcpy(value, bytes.data() + address, 4);
        return true;
    }
    bool Write8(uint32_t address, uint8_t value) {
        if (address >= bytes.size() || address == unavailable) return false;
        bytes[address] = value;
        return true;
    }
    bool Write16(uint32_t address, uint16_t value) {
        if (address > bytes.size() - 2) return false;
        std::memcpy(bytes.data() + address, &value, 2);
        return true;
    }
    bool Write32(uint32_t address, uint32_t value) {
        if (address > bytes.size() - 4) return false;
        std::memcpy(bytes.data() + address, &value, 4);
        return true;
    }
};

int main() {
    using Comfort::Ammo;
    constexpr uint32_t save = Comfort::SaveContext;
    Memory memory;
    struct AmmoCase { Ammo type; uint32_t slot, shift; uint8_t item, base; };
    const AmmoCase cases[] = {
        {Ammo::Bombs, 2, 3, 2, 20}, {Ammo::Nuts, 1, 20, 1, 20},
        {Ammo::Seeds, 6, 14, 6, 30}, {Ammo::Sticks, 0, 17, 0, 10},
        {Ammo::Arrows, 3, 0, 3, 30},
    };
    for (const auto& test : cases) {
        memory.bytes[save + 0x8C + test.slot] = test.item;
        for (uint32_t level = 0; level < 8; ++level) {
            memory.Write32(save + 0xB8, level << test.shift);
            memory.bytes[save + 0xA6 + test.slot] = 0;
            auto expected = memory.bytes;
            const bool valid = level >= 1 && level <= 3;
            if (valid) expected[save + 0xA6 + test.slot] = test.base + 10 * (level - 1);
            assert(Comfort::RefillAmmo(memory, test.type) == valid);
            assert(memory.bytes == expected);
        }
        memory.Write32(save + 0xB8, 1U << test.shift);
        memory.bytes[save + 0x8C + test.slot] = 0xFF;
        const auto before = memory.bytes;
        assert(!Comfort::RefillAmmo(memory, test.type));
        assert(memory.bytes == before);
    }
    memory.Write32(save + 0xB8, 3);
    for (uint8_t item = 0x38; item <= 0x3A; ++item) {
        memory.bytes[save + 0x8F] = item;
        assert(Comfort::RefillAmmo(memory, Ammo::Arrows));
        assert(memory.bytes[save + 0xA9] == 50);
    }
    memory.bytes[save + 0x9A] = 0x10;
    assert(Comfort::RefillAmmo(memory, Ammo::Beans));
    assert(memory.bytes[save + 0xB4] == 10);
    memory.bytes[save + 0x9A] = 0xFF;
    auto before = memory.bytes;
    assert(!Comfort::RefillAmmo(memory, Ammo::Beans));
    assert(!Comfort::RefillAmmo(memory, static_cast<Ammo>(99)));
    assert(memory.bytes == before);

    for (const uint16_t time : {0x4000, 0x5800, 0x7000, 0xC000, 0xD000}) {
        auto expected = memory.bytes;
        std::memcpy(expected.data() + save + 0xC, &time, 2);
        std::memcpy(expected.data() + save + 0x15A8, &time, 2);
        const uint32_t night = time >= 0xC000 ? 1 : 0;
        std::memcpy(expected.data() + save + 0x10, &night, 4);
        assert(Comfort::SetTime(memory, time));
        assert(memory.bytes == expected);
    }
    before = memory.bytes;
    assert(!Comfort::SetTime(memory, 0xFFFF));
    memory.unavailable = save + 0x15A9;
    assert(!Comfort::SetTime(memory, 0x5800));
    assert(memory.bytes == before);
    memory.unavailable = UINT32_MAX;

    constexpr uint32_t player = 0x1000;
    memory.bytes[player + 0x1AA] = 0xFF;
    for (uint32_t slot = 0; slot < 4; ++slot) {
        memory.bytes[save + 0x9E + slot] = 0x14;
        for (const uint32_t block : {0x54U, 0x60U, 0x80U}) {
            for (uint32_t button = 0; button < 4; ++button) {
                memory.bytes[save + block + 5 + button] = button == slot ? 0x12 + slot : 0xFF;
            }
        }
        for (uint8_t content = 0x14; content <= 0x20; ++content) {
            auto expected = memory.bytes;
            expected[save + 0x9E + slot] = content;
            for (const uint32_t block : {0x54U, 0x60U, 0x80U}) {
                expected[save + block + 1 + slot] = content;
            }
            assert(Comfort::SetBottle(memory, slot, content, player));
            assert(memory.bytes == expected);
        }
    }
    before = memory.bytes;
    assert(!Comfort::SetBottle(memory, 4, 0x14, player));
    assert(!Comfort::SetBottle(memory, 0, 0x13, player));
    assert(!Comfort::SetBottle(memory, 0, 0x21, player));
    assert(!Comfort::SetBottle(memory, 0, 0x14, 0));
    assert(!Comfort::SetBottle(memory, 0, 0x14, UINT32_MAX));
    assert(memory.bytes == before);
    memory.bytes[save + 0x9E] = 0xFF;
    before = memory.bytes;
    assert(!Comfort::SetBottle(memory, 0, 0x18, player));
    assert(memory.bytes == before);
    memory.bytes[save + 0x9E] = 0x14;
    for (uint8_t held = 0x14; held <= 0x20; ++held) {
        memory.bytes[player + 0x1AA] = held;
        before = memory.bytes;
        assert(!Comfort::SetBottle(memory, 0, 0x18, player));
        assert(memory.bytes == before);
    }
    memory.bytes[player + 0x1AA] = 0xFF;
    memory.unavailable = save + 0x88;
    before = memory.bytes;
    assert(!Comfort::SetBottle(memory, 0, 0x18, player));
    assert(memory.bytes == before);
    std::cout << "Passed: owned ammo, native capacities, time presets, bottle slots, equipment caches, invalid mappings.\n";
}
