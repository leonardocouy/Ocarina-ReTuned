// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "levitation.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>

namespace Comfort {
enum class Feedback { None, Damage, Landing, Reset };

class FeedbackTracker {
  public:
    template<class Memory>
    Feedback Update(Memory& memory, uint32_t global) {
        uint32_t player = 0, velocityBits = 0;
        uint16_t health = 0, ground = 0;
        if (!ReadPlayer(memory, global, &player) || player > UINT32_MAX - 0x91 ||
            !memory.Read16(SaveContext + 0x44, &health) ||
            !memory.Read16(player + 0x90, &ground) ||
            !memory.Read32(player + 0x64, &velocityBits)) return Reset();
        const float velocity = std::bit_cast<float>(velocityBits);
        if (!std::isfinite(velocity)) return Reset();
        const bool grounded = (ground & 1) != 0;
        if (global != lastGlobal || player != lastPlayer) {
            lastGlobal = global;
            lastPlayer = player;
            lastHealth = health;
            wasGrounded = grounded;
            fallSpeed = grounded ? 0 : velocity;
            return Feedback::Reset;
        }
        const bool damage = health < lastHealth;
        const bool landing = grounded && !wasGrounded && fallSpeed < -8.f;
        lastHealth = health;
        wasGrounded = grounded;
        fallSpeed = grounded ? 0 : std::min(fallSpeed, velocity);
        return damage ? Feedback::Damage : landing ? Feedback::Landing : Feedback::None;
    }
  private:
    Feedback Reset() {
        const bool active = lastPlayer != 0;
        lastGlobal = lastPlayer = 0;
        fallSpeed = 0;
        return active ? Feedback::Reset : Feedback::None;
    }
    uint32_t lastGlobal = 0, lastPlayer = 0;
    uint16_t lastHealth = 0;
    bool wasGrounded = true;
    float fallSpeed = 0;
};
}
