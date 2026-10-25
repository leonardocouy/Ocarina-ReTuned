# SPDX-License-Identifier: GPL-2.0-or-later
"""Add a saved host motion-recenter binding to the native Controls panel."""
from pathlib import Path
from apply_native_port import edit


def apply(engine):
    runtime = Path(engine) / 'tools/oot3d/native_game_runtime'
    (runtime / 'retuned_motion_recenter.h').write_text(
        Path(__file__).with_name('motion_recenter.h').read_text())
    edit(runtime / 'oot3d_native_control_config.h',
         '  LookRight,\n  Count,', '  LookRight,\n  RecenterMotion,\n  Count,')
    config = runtime / 'oot3d_native_control_config.cpp'
    edit(config, '{NativeControlAction::LookRight, "look_right"},',
         '{NativeControlAction::LookRight, "look_right"},\n'
         '        {NativeControlAction::RecenterMotion, "recenter_motion"},')
    edit(config, '  SetGamepadBinding(config, NativeControlAction::A,',
         '  SetKeyboardBinding(config, NativeControlAction::RecenterMotion, NativeKeyboardKey::T);\n'
         '  SetGamepadBinding(config, NativeControlAction::RecenterMotion, NativeGamepadButton::RightStick);\n\n'
         '  SetGamepadBinding(config, NativeControlAction::A,')
    panel = runtime / 'oot3d_native_controls_settings_panel.cpp'
    edit(panel, '    "Look up", "Look down", "Look left", "Look right",',
         '    "Look up", "Look down", "Look left", "Look right", "Recenter motion",')
    edit(panel, 'constexpr std::array<BindingGroup, 5> kBindingGroups',
         'constexpr std::array<BindingGroup, 6> kBindingGroups')
    edit(panel, '{"Look directions", 21, 25},',
         '{"Look directions", 21, 25}, {"Motion aiming", 25, 26},')
    inputs = runtime / 'oot3d_native_a32_input.cpp'
    edit(inputs, '        {true, DigitalControl::CStickRight},',
         '        {true, DigitalControl::CStickRight},\n        {false},')
    shared = runtime / 'oot3d_native_a32_window_shared.h'
    edit(shared, '#pragma once', '#pragma once\n#include "retuned_motion_recenter.h"')
    edit(shared, 'ThreeDsRecomp::Input::VirtualMotionState VirtualMotion;',
         'ThreeDsRecomp::Input::VirtualMotionState VirtualMotion;\n'
         '  RetunedFrontend::MotionRecenter MotionRecenter;')
    polling = runtime / 'oot3d_native_a32_window_input.cpp'
    edit(polling, '    pollingState.VirtualMotion.ResetController();',
         '    pollingState.VirtualMotion.ResetController();\n'
         '    pollingState.MotionRecenter = {};')
    edit(polling, '  auto frame = MapNativeControlInput(config, host, aimTransform,',
         '  const bool recenterMotion = host.Actions[static_cast<size_t>(NativeControlAction::RecenterMotion)];\n'
         '  host.ControllerMotion = pollingState.MotionRecenter.Apply(\n'
         '      host.ControllerMotion, recenterMotion, config.GyroscopeBiasDegreesPerSecond,\n'
         '      host.SamplePeriodSeconds, guestRefreshWillConsume);\n'
         '  if (recenterMotion) {\n'
         '    config.RightStickAimMaximumDegreesPerSecond = 0;\n'
         '    host.MouseDeltaX = host.MouseDeltaY = 0;\n'
         '  }\n'
         '  auto frame = MapNativeControlInput(config, host, aimTransform,')
    edit(polling, '  if (nativeFrontendTouchEnabled && !hostGuiVisible) {',
         '  if (recenterMotion) frame.CStick = {};\n'
         '  if (nativeFrontendTouchEnabled && !hostGuiVisible) {')
    edit(runtime / 'oot3d_native_a32_input_tests.cpp',
         '        ++nativeTouchShortcuts;',
         '''        if (action == NativeControlAction::RecenterMotion) {
            NativeControlHostInputState host;
            host.Actions[index] = true;
            auto frame = MapNativeControlInput(keyboardMouse, host);
            ApplyNativeControlShortcutTouch(host, frame);
            Require(frame.Hid.Buttons == 0 && !frame.Hid.TouchPressed,
                    "motion recenter leaked a game button or touch");
            continue;
        }
        ++nativeTouchShortcuts;''')
