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
    bool Write8(uint32_t address, uint8_t value) {
        if (!bytes.contains(address)) return false;
        bytes[address] = value;
        return true;
    }
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
    auto status = Comfort::ReadFishing(memory, Global);
    assert(status && !status->Hooked && status->Risk == 1.f);
    assert(memory.bytes == before);
    for (const auto [address, value] : std::map<uint32_t, uint32_t>{
        {Global + 0x104, 72}, {Comfort::FishingState + 0x1E, 3},
        {Comfort::FishingState + 0x3C, 0}, {Comfort::FishingState + 0xCC, UINT32_MAX},
        {0x30000, 0x95}, {0x3013C, 0}, {0x301B0, 4},
        {0x5043E8, 1}, {Global + 0x22A0, 1}, {Global + 0x5C2D, 1},
        {Comfort::SaveContext + 0x44, 0}}) {
        memory = Ready(); Bite(memory);
        memory.Put<uint32_t>(address, value);
        assert(!Comfort::ReadFishing(memory, Global));
    }
    memory = Ready(); Bite(memory);
    memory.Put<uint8_t>(Comfort::FishingState + 0x1A, 1);
    memory.Put<uint16_t>(Comfort::FishingState + 0x3C, 0);
    memory.Put<uint16_t>(Comfort::FishingState + 0x3A, 100);
    memory.Put<uint16_t>(Comfort::FishingState + 0x26, 100);
    memory.Put<uint8_t>(Comfort::FishingState + 0xC, 0);
    memory.Put<uint32_t>(0x51B2F4, 0x40000);
    memory.Put<uint16_t>(0x40110, 2);
    status = Comfort::ReadFishing(memory, Global);
    assert(status && status->Hooked && status->Risk == 0.f);
    memory.Put<uint16_t>(Comfort::FishingState + 0x3A, 25);
    status = Comfort::ReadFishing(memory, Global);
    assert(status && status->Risk == .75f);
    memory.Put<uint16_t>(Comfort::FishingState + 0x3A, 0);
    assert(Comfort::ReadFishing(memory, Global)->Risk == 1.f);
    memory.Put<uint16_t>(Comfort::FishingState + 0x3A, 100);
    memory.Put<uint8_t>(Comfort::FishingState + 0xC, 1);
    assert(Comfort::ReadFishing(memory, Global)->Risk == 1.f);
    memory.Put<uint8_t>(Comfort::FishingState + 0xC, 0);
    memory.Put<uint16_t>(Comfort::FishingState + 0x26, 8925);
    assert(Comfort::ReadFishing(memory, Global)->Risk == .5f);
    memory.Put<uint16_t>(Comfort::FishingState + 0x26, 9000);
    assert(Comfort::ReadFishing(memory, Global)->Risk == 1.f);
    memory.Put<uint16_t>(0x40110, 0);
    assert(!Comfort::ReadFishing(memory, Global));
    assert(!Comfort::ReadFishing(memory, UINT32_MAX));

    Comfort::FishingFeedback feedback;
    const Comfort::FishingStatus steady{0.f, true}, danger{1.f, true}, bite{1.f, false};
    assert(!feedback.Update({}, 0));
    assert(feedback.Update(bite, 0));
    assert(!feedback.Update(bite, 100));
    assert(feedback.Update(steady, 100));
    assert(!feedback.Update(steady, 1099));
    assert(feedback.Update(danger, 1100));
    assert(!feedback.Update(danger, 1432));
    assert(feedback.Update(danger, 1433));
    assert(!feedback.Update({}, 1500));
    assert(feedback.Update(steady, 1501));
    assert(feedback.Update(danger, 1901));
    memory = Ready();
    memory.Put<uint16_t>(Global + 0x104, 73);
    memory.Put<uint16_t>(Comfort::FishingState + 0x30, 2);
    memory.Put<uint16_t>(Comfort::FishingState + 0x1E, 0);
    memory.Put<uint8_t>(Comfort::FishingState + 0x16, 1);
    uint8_t equipped = 0;
    Comfort::SetSinkingLure(memory, Global, false);
    assert(memory.Read8(Comfort::FishingState + 0x16, &equipped) && equipped == 0);
    Comfort::SetSinkingLure(memory, Global, true);
    assert(memory.Read8(Comfort::FishingState + 0x16, &equipped) && equipped == 2);
    memory.Put<uint16_t>(Comfort::FishingState + 0x1E, 4);
    Comfort::SetSinkingLure(memory, Global, false);
    assert(memory.Read8(Comfort::FishingState + 0x16, &equipped) && equipped == 2);
    memory.Put<uint16_t>(Comfort::FishingState + 0x1E, 0);
    memory.Put<uint32_t>(0x5043E8, 1);
    Comfort::SetSinkingLure(memory, Global, false);
    assert(memory.Read8(Comfort::FishingState + 0x16, &equipped) && equipped == 2);
    memory.Put<uint32_t>(0x5043E8, 0);
    Comfort::SetSinkingLure(memory, Global, false);
    assert(memory.Read8(Comfort::FishingState + 0x16, &equipped) && equipped == 0);
    memory.Put<uint16_t>(Global + 0x104, 72);
    Comfort::SetSinkingLure(memory, Global, true);
    assert(memory.Read8(Comfort::FishingState + 0x16, &equipped) && equipped == 0);
    memory.Put<uint16_t>(Global + 0x104, 73);
    memory.Put<uint16_t>(Comfort::FishingState + 0x30, 0);
    Comfort::SetSinkingLure(memory, Global, true);
    assert(memory.Read8(Comfort::FishingState + 0x16, &equipped) && equipped == 0);
    std::cout << "Passed: hooked fights remain active; slack and deadlines raise risk; vibration repeats faster in danger and stops when inactive. Sinking lure changes wait for an idle fishing session.\n";
}
