# SPDX-License-Identifier: GPL-2.0-or-later
from pathlib import Path
import argparse
import shutil
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'reframed'))
from apply_native_port import edit


def apply(engine):
    source = Path(__file__).resolve().parent
    engine = Path(engine)
    runtime = engine / 'tools/oot3d/native_game_runtime'
    shutil.copy2(source.parent / 'cheats/api.h', runtime / 'retuned_comfort_api.h')
    shutil.copy2(source.parent / 'cheats/destinations.h', runtime / 'destinations.h')
    shutil.copy2(source / 'cheat_notice.h', runtime / 'retuned_cheat_notice.h')
    shutil.copy2(source / 'cheats_panel.h', runtime / 'retuned_cheats_panel.h')
    header = runtime / 'oot3d_native_direct_aot.h'
    edit(header, '#include "oot3d_native_whole_aot_runtime.h"',
         '#include "oot3d_native_whole_aot_runtime.h"\n#include "retuned_comfort_api.h"')
    edit(header, 'bool Oot3dWholeAotPluginV2Available() noexcept;',
         'const RetunedComfort::Api* ComfortCheats() noexcept;\n\nbool Oot3dWholeAotPluginV2Available() noexcept;')
    implementation = header.with_suffix('.cpp')
    text = implementation.read_text()
    marker = 'bool Oot3dWholeAotPluginV2Available() noexcept {'
    getter = 'const RetunedComfort::Api* ComfortCheats() noexcept {'
    start = text.index(getter) if getter in text else text.index(marker)
    end = text.index(marker, start)
    replacement = '''const RetunedComfort::Api* ComfortCheats() noexcept {
#if defined(__APPLE__)
  const auto* api = QueryTitle<RetunedComfort::Api>("retuned_comfort_query", 4);
  if (api && api->Version == 4 && api->Size == sizeof(RetunedComfort::Api) &&
      api->Get && api->Set && api->Request && api->LastError && api->Notice && api->Skulltula) return api;
#endif
  return nullptr;
}

'''
    implementation.write_text(text[:start] + replacement + text[end:])
    frame = runtime / 'oot3d_native_a32_window_frame.cpp'
    edit(frame, '#include "oot3d_native_a32_window_session.h"',
         '#include "oot3d_native_a32_window_session.h"\n#include "retuned_cheat_notice.h"')
    draw = '  gui->StartDraw();\n  RetunedFrontend::DrawCheatNotice(*gui);'
    text = frame.read_text()
    if draw not in text:
        edit(frame, '  gui->StartDraw();', draw)
    edit(frame, draw, draw + '\n  RetunedFrontend::DrawSkulltulaIndicator(*gui, uiLifecycleBridge, nativeUiTextureProviderStorage ? &*nativeUiTextureProviderStorage : nullptr);')
    setup = runtime / 'oot3d_native_a32_window_setup.cpp'
    if '#include "retuned_reframed_settings.h"' not in setup.read_text():
        return
    edit(setup, '#include "retuned_reframed_settings.h"',
         '#include "retuned_reframed_settings.h"\n#include "retuned_cheats_panel.h"')
    marker = '  // F1 order: the scene lights join Lighting > Lights & Shadows; then'
    registration = '  applicationSettingsTabs.push_back(std::make_shared<RetunedFrontend::CheatsPanel>());\n'
    text = setup.read_text().replace(registration, '')
    if text.count(marker) != 1:
        raise ValueError('Cheats panel registration context changed.')
    setup.write_text(text.replace(marker, registration + marker))



if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('engine', type=Path)
    args = parser.parse_args()
    apply(args.engine)
