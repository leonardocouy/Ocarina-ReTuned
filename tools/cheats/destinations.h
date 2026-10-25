// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <array>
#include <cstdint>

namespace RetunedComfort {
enum class WarpCategory { Overworld, Dungeon };
struct WarpDestination {
    const char* Name;
    WarpCategory Category;
    uint16_t Entrance;
};

// Adapted from OcarinaCTRComposer (MIT), copyright 2026 samabr (samaBR85).
// Entrance IDs were checked against the supported prepared game's entrance table.
inline constexpr std::array Destinations{
    WarpDestination{"Reload current scene", WarpCategory::Overworld, 0xFFFF},
    WarpDestination{"Kokiri Forest", WarpCategory::Overworld, 0x00EE},
    WarpDestination{"Lost Woods", WarpCategory::Overworld, 0x011E},
    WarpDestination{"Link's House", WarpCategory::Overworld, 0x0272},
    WarpDestination{"Hyrule Field", WarpCategory::Overworld, 0x00CD},
    WarpDestination{"Market", WarpCategory::Overworld, 0x00B1},
    WarpDestination{"Temple of Time", WarpCategory::Overworld, 0x0053},
    WarpDestination{"Lon Lon Ranch", WarpCategory::Overworld, 0x0157},
    WarpDestination{"Kakariko Village", WarpCategory::Overworld, 0x00DB},
    WarpDestination{"Death Mountain Trail", WarpCategory::Overworld, 0x013D},
    WarpDestination{"Death Mountain Crater", WarpCategory::Overworld, 0x0147},
    WarpDestination{"Goron City", WarpCategory::Overworld, 0x014D},
    WarpDestination{"Zora's River", WarpCategory::Overworld, 0x00EA},
    WarpDestination{"Zora's Domain", WarpCategory::Overworld, 0x0108},
    WarpDestination{"Zora's Fountain", WarpCategory::Overworld, 0x010E},
    WarpDestination{"Lake Hylia", WarpCategory::Overworld, 0x0102},
    WarpDestination{"Gerudo Valley", WarpCategory::Overworld, 0x0117},
    WarpDestination{"Haunted Wasteland", WarpCategory::Overworld, 0x0130},
    WarpDestination{"Desert Colossus", WarpCategory::Overworld, 0x0123},
    WarpDestination{"Deku Tree", WarpCategory::Dungeon, 0x0000},
    WarpDestination{"Dodongo's Cavern", WarpCategory::Dungeon, 0x0004},
    WarpDestination{"Jabu-Jabu's Belly", WarpCategory::Dungeon, 0x0028},
    WarpDestination{"Forest Temple", WarpCategory::Dungeon, 0x0169},
    WarpDestination{"Fire Temple", WarpCategory::Dungeon, 0x0165},
    WarpDestination{"Water Temple", WarpCategory::Dungeon, 0x0010},
    WarpDestination{"Shadow Temple", WarpCategory::Dungeon, 0x0037},
    WarpDestination{"Spirit Temple", WarpCategory::Dungeon, 0x0082},
    WarpDestination{"Ice Cavern", WarpCategory::Dungeon, 0x0088},
    WarpDestination{"Gerudo Training Ground", WarpCategory::Dungeon, 0x0008},
    WarpDestination{"Ganon's Castle", WarpCategory::Dungeon, 0x0467},
    WarpDestination{"Bottom of the Well", WarpCategory::Dungeon, 0x0098},
};
}
