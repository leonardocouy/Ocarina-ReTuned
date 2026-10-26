// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "levitation.h"
#include <array>
#include <bit>
#include <cmath>
#include <optional>

namespace Comfort {
inline constexpr float SkulltulaRange = 1200.f;
struct SkulltulaDetection {
    float Distance;
    bool Token;
};

template<class Memory>
bool ActorPosition(Memory& memory, uint32_t actor, std::array<float, 3>& position) {
    if (actor > UINT32_MAX - 0x33) return false;
    for (uint32_t i = 0; i < position.size(); ++i) {
        uint32_t bits = 0;
        if (!memory.Read32(actor + 0x28 + i * 4, &bits)) return false;
        position[i] = std::bit_cast<float>(bits);
        if (!std::isfinite(position[i])) return false;
    }
    return true;
}

template<class Memory>
std::optional<SkulltulaDetection> FindSkulltula(Memory& memory, uint32_t global) {
    uint32_t player = 0;
    if (!ReadPlayer(memory, global, &player)) return {};
    std::array<float, 3> origin{};
    uint8_t total = 0;
    uint32_t night = 0;
    if (!ActorPosition(memory, player, origin) ||
        !memory.Read8(global + 0x2094, &total) ||
        !memory.Read32(SaveContext + 0x10, &night)) return {};
    std::optional<SkulltulaDetection> nearest;
    uint32_t visited = 0;
    for (uint32_t category = 0; category < 12; ++category) {
        uint32_t count = 0, actor = 0;
        const uint32_t list = global + 0x2098 + category * 8;
        if (!memory.Read32(list, &count) || !memory.Read32(list + 4, &actor) ||
            count > total - visited) return {};
        visited += count;
        for (uint32_t i = 0; i < count; ++i) {
            uint16_t id = 0, params = 0;
            uint8_t type = 0;
            uint32_t next = 0, init = 0, update = 0;
            if (!actor || actor > UINT32_MAX - 0x13F ||
                !memory.Read16(actor, &id) || !memory.Read8(actor + 2, &type) || type != category ||
                !memory.Read32(actor + 0x130, &next)) return {};
            const bool token = id == 0x19C;
            if (id == 0x95 || token) {
                if (!memory.Read16(actor + 0x1C, &params) ||
                    !memory.Read32(actor + 0x134, &init) ||
                    !memory.Read32(actor + 0x13C, &update)) return {};
                const uint32_t index = (params >> 8) & 0x1F;
                const uint8_t bit = params & 0xFF;
                if (!init && update && (token || (params & 0xE000)) &&
                    (token || !(params & 0x4000) || night) &&
                    index < 22 && bit && !(bit & (bit - 1))) {
                    uint8_t collected = 0;
                    std::array<float, 3> position{};
                    if (!memory.Read8(SaveContext + 0xEB4 + index, &collected)) return {};
                    if (!(collected & bit) && ActorPosition(memory, actor, position)) {
                        const double dx = double(position[0]) - origin[0];
                        const double dy = double(position[1]) - origin[1];
                        const double dz = double(position[2]) - origin[2];
                        const double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
                        if (distance <= SkulltulaRange && (!nearest || distance < nearest->Distance))
                            nearest = SkulltulaDetection{static_cast<float>(distance), token};
                    }
                }
            }
            actor = next;
        }
        if (actor) return {};
    }
    return nearest;
}
}
