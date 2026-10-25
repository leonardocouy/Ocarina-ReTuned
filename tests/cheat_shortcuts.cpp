// SPDX-License-Identifier: GPL-2.0-or-later
#include "../tools/cheats/shortcuts.h"
#include <cassert>
#include <initializer_list>

int main() {
    Comfort::LevitationShortcut shortcut;
    assert(!shortcut.Update(0x300, true));
    assert(shortcut.Update(0x340, true));
    assert(!shortcut.Update(0x340, true));
    assert(!shortcut.Update(0x300, true));
    assert(shortcut.Update(0x340, true));
    assert(!shortcut.Update(0, true));
    assert(!shortcut.Update(0x40, true));
    assert(!shortcut.Update(0x340, true));
    assert(!shortcut.Update(0, false));
    assert(!shortcut.Update(0x340, false));
    assert(!shortcut.Update(0x340, true));
    assert(!shortcut.Update(0x300, true));
    assert(shortcut.Update(0x340, true));
    for (uint32_t buttons : {0x40U, 0x140U, 0x240U}) {
        assert(!shortcut.Update(0, true));
        assert(!shortcut.Update(buttons, true));
    }
}
