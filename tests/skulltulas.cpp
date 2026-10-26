// SPDX-License-Identifier: GPL-2.0-or-later
#include "../tools/cheats/skulltulas.h"
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
    memory.Put<uint32_t>(Comfort::SaveContext + 0x10, 1);
    memory.Put<uint8_t>(Global + 0x2094, 1);
    for (uint32_t category = 0; category < 12; ++category) {
        memory.Put<uint32_t>(Global + 0x2098 + category * 8, category == 2 ? 1 : 0);
        memory.Put<uint32_t>(Global + 0x209C + category * 8, category == 2 ? Player : 0);
    }
    memory.Put<uint32_t>(Player + 0x130, 0);
    for (uint32_t i = 0; i < 3; ++i) memory.Put<uint32_t>(Player + 0x28 + i * 4, 0);
    return memory;
}


constexpr uint32_t Spider = 0x30000, Token = 0x40000;

void Add(Memory& memory, uint32_t actor, uint16_t id, uint16_t params, float x, float y = 0) {
    const uint32_t list = Global + 0x2098 + 5 * 8;
    uint32_t head = 0, count = 0;
    uint8_t total = 0;
    assert(memory.Read32(list, &count) && memory.Read32(list + 4, &head) && memory.Read8(Global + 0x2094, &total));
    memory.Put<uint32_t>(list, count + 1);
    memory.Put<uint32_t>(list + 4, actor);
    memory.Put<uint8_t>(Global + 0x2094, total + 1);
    memory.Put<uint16_t>(actor, id);
    memory.Put<uint8_t>(actor + 2, 5);
    memory.Put<uint16_t>(actor + 0x1C, params);
    memory.Put<uint32_t>(actor + 0x130, head);
    memory.Put<uint32_t>(actor + 0x134, 0);
    memory.Put<uint32_t>(actor + 0x13C, 0x100000);
    memory.Put<uint32_t>(actor + 0x28, std::bit_cast<uint32_t>(x));
    memory.Put<uint32_t>(actor + 0x2C, std::bit_cast<uint32_t>(y));
    memory.Put<uint32_t>(actor + 0x30, 0);
    memory.Put<uint8_t>(Comfort::SaveContext + 0xEB4 + ((params >> 8) & 0x1F), 0);
}

int main() {
    auto memory = Ready();
    assert(!Comfort::FindSkulltula(memory, Global));
    Add(memory, Spider, 0x95, 0, 100);
    assert(!Comfort::FindSkulltula(memory, Global));
    memory.Put<uint16_t>(Spider + 0x1C, 0x8001);
    memory.Put<uint8_t>(Comfort::SaveContext + 0xEB4, 0);
    const auto before = memory.bytes;
    auto detection = Comfort::FindSkulltula(memory, Global);
    assert(detection && detection->Distance == 100 && !detection->Token);
    assert(memory.bytes == before);
    Add(memory, Token, 0x19C, 0x0102, 30, 40);
    detection = Comfort::FindSkulltula(memory, Global);
    assert(detection && detection->Distance == 50 && detection->Token);
    memory.Put<uint8_t>(Comfort::SaveContext + 0xEB5, 2);
    detection = Comfort::FindSkulltula(memory, Global);
    assert(detection && detection->Distance == 100 && !detection->Token);
    memory.Put<uint8_t>(Comfort::SaveContext + 0xEB4, 1);
    assert(!Comfort::FindSkulltula(memory, Global));

    for (uint32_t field : {Spider + 0x134, 0x5043E8U, Global + 0x22A0, Global + 0x5C2D}) {
        memory = Ready(); Add(memory, Spider, 0x95, 0x8001, 100);
        memory.Put<uint32_t>(field, 1);
        assert(!Comfort::FindSkulltula(memory, Global));
    }
    memory = Ready(); Add(memory, Spider, 0x95, 0xC001, 100);
    memory.Put<uint32_t>(Comfort::SaveContext + 0x10, 0);
    assert(!Comfort::FindSkulltula(memory, Global));
    memory.Put<uint32_t>(Comfort::SaveContext + 0x10, 1);
    assert(Comfort::FindSkulltula(memory, Global));
    for (uint32_t value : {0U, UINT32_MAX, 0x50000U}) {
        memory = Ready(); Add(memory, Spider, 0x95, 0x8001, 100);
        memory.Put<uint32_t>(Global + 0x20C4, value);
        assert(!Comfort::FindSkulltula(memory, Global));
    }
    memory = Ready(); Add(memory, Spider, 0x95, 0x8001, 100);
    memory.Put<uint32_t>(Spider + 0x13C, 0);
    assert(!Comfort::FindSkulltula(memory, Global));
    memory = Ready(); Add(memory, Spider, 0x95, 0x8001, Comfort::SkulltulaRange + 1);
    assert(!Comfort::FindSkulltula(memory, Global));
    memory.Put<uint32_t>(Spider + 0x28, std::bit_cast<uint32_t>(Comfort::SkulltulaRange));
    assert(Comfort::FindSkulltula(memory, Global));
    memory.Put<uint32_t>(Spider + 0x2C, std::bit_cast<uint32_t>(Comfort::SkulltulaRange));
    assert(!Comfort::FindSkulltula(memory, Global));
    memory.Put<uint32_t>(Spider + 0x28, 0x7FC00000);
    assert(!Comfort::FindSkulltula(memory, Global));
    memory = Ready(); Add(memory, Spider, 0x95, 0x8001, 100);
    memory.Put<uint32_t>(Spider + 0x130, Spider);
    assert(!Comfort::FindSkulltula(memory, Global));
    memory = Ready(); Add(memory, Spider, 0x95, 0x8001, 100);
    memory.Put<uint32_t>(Global + 0x20C0, UINT32_MAX);
    assert(!Comfort::FindSkulltula(memory, Global));
    for (uint16_t params : {uint16_t(0x9F01), uint16_t(0x8000), uint16_t(0x8003)}) {
        memory = Ready(); Add(memory, Spider, 0x95, params, 100);
        assert(!Comfort::FindSkulltula(memory, Global));
    }
    memory = Ready(); Add(memory, Spider, 0x95, 0x8001, 100);
    memory.bytes.erase(Spider + 0x131);
    assert(!Comfort::FindSkulltula(memory, Global));
    assert(!Comfort::FindSkulltula(memory, UINT32_MAX));
    std::cout << "Passed: Gold Skulltulas and tokens use 3D proximity; ordinary spiders, collected flags, invalid lists, and inactive play stay quiet.\n";
}
