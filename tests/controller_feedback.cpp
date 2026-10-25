// SPDX-License-Identifier: GPL-2.0-or-later
#include "../tools/cheats/feedback.h"
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

void Motion(Memory& memory, bool grounded, float velocity) {
    memory.Put<uint16_t>(Player + 0x90, grounded ? 1 : 0);
    memory.Put<uint32_t>(Player + 0x64, std::bit_cast<uint32_t>(velocity));
}

int main() {
    using Comfort::Feedback;
    Comfort::FeedbackTracker tracker;
    auto memory = Ready();
    assert(tracker.Update(memory, Global) == Feedback::Reset);
    memory.Put<uint16_t>(Comfort::SaveContext + 0x44, 40);
    assert(tracker.Update(memory, Global) == Feedback::Damage);
    assert(tracker.Update(memory, Global) == Feedback::None);
    memory.Put<uint16_t>(Comfort::SaveContext + 0x44, 48);
    assert(tracker.Update(memory, Global) == Feedback::None);

    Motion(memory, false, -4.f);
    assert(tracker.Update(memory, Global) == Feedback::None);
    Motion(memory, true, 0.f);
    assert(tracker.Update(memory, Global) == Feedback::None);
    Motion(memory, false, -12.f);
    assert(tracker.Update(memory, Global) == Feedback::None);
    Motion(memory, false, -2.f);
    assert(tracker.Update(memory, Global) == Feedback::None);
    Motion(memory, true, 0.f);
    assert(tracker.Update(memory, Global) == Feedback::Landing);
    assert(tracker.Update(memory, Global) == Feedback::None);
    Motion(memory, false, -12.f);
    assert(tracker.Update(memory, Global) == Feedback::None);
    Motion(memory, true, 0.f);
    memory.Put<uint16_t>(Comfort::SaveContext + 0x44, 40);
    assert(tracker.Update(memory, Global) == Feedback::Damage);

    for (uint32_t address : {0x5043E8U, Global + 0x22A0, Global + 0x5C2D}) {
        memory = Ready();
        tracker.Update(memory, Global);
        memory.Put<uint32_t>(address, 1);
        assert(tracker.Update(memory, Global) == Feedback::Reset);
        assert(tracker.Update(memory, Global) == Feedback::None);
        memory.Put<uint32_t>(address, 0);
        memory.Put<uint16_t>(Comfort::SaveContext + 0x44, 8);
        assert(tracker.Update(memory, Global) == Feedback::Reset);
        assert(tracker.Update(memory, Global) == Feedback::None);
    }
    memory = Ready();
    tracker.Update(memory, Global);
    memory.bytes.erase(Player + 0x91);
    assert(tracker.Update(memory, Global) == Feedback::Reset);
    memory = Ready();
    tracker.Update(memory, Global);
    memory.Put<uint32_t>(Player + 0x64, 0x7FC00000);
    assert(tracker.Update(memory, Global) == Feedback::Reset);
    memory = Ready();
    tracker.Update(memory, Global);
    assert(tracker.Update(memory, 0) == Feedback::Reset);
    assert(tracker.Update(memory, Global) == Feedback::Reset);
    std::cout << "Passed: damage and hard landings pulse once; healing, short falls, pauses, transitions, and invalid memory stay quiet.\n";
}
