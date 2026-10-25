// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>

namespace Comfort {
class LevitationShortcut {
  public:
    bool Update(uint32_t buttons, bool playing) {
        constexpr uint32_t up = 0x40, shoulders = 0x300;
        const bool activate = playing && (buttons & shoulders) == shoulders &&
            (buttons & up) && !(previous & up);
        previous = buttons;
        return activate;
    }
  private:
    uint32_t previous = 0;
};
}
