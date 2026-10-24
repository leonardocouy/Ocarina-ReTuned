// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>

namespace RetunedComfort {
enum class Cheat : uint32_t { InfiniteRupees, InstantDialogue };
struct Api {
    uint32_t Version;
    uint32_t Size;
    bool (*Get)(Cheat, bool*) noexcept;
    bool (*Set)(Cheat, bool) noexcept;
    const char* (*LastError)() noexcept;
};
}
