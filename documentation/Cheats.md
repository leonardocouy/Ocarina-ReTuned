# Local cheats on macOS

This fork adds two optional cheats:

- **Infinite rupees:** keep the balance at 999 during play.
- **Instant dialogue:** show each text page immediately. Advance pages and choose answers yourself.

These cheats do not skip entire conversations or cutscenes. Turning off infinite rupees stops refills. It does not restore the previous balance.

## Supported installation

The module was verified on Apple silicon with a game prepared by **Preview 0.1.0**. It uses the pinned TriAevum runtime at `17218b0cfdb944645bb5cde5249f313dca7e53ce`.

The installer checks the prepared code's SHA-256 identity and the runtime before changing the launch profile. It rejects other images. A newly prepared 0.2.0 game has not been validated.

The fork follows current upstream source, while the cheat hooks remain limited to this verified game image. Keep your existing prepared installation when building a newer interface.

The source contains no ROM, save, prepared module, or extracted game data. Build the small wrapper locally against the headers in your installed app.

## Install

Requirements: Python 3.11 or later, Apple C++ command-line tools, and an installed ReTuned app with its preparation toolkit.

Save your game, close ReTuned, and keep a separate backup of your saves. From the repository root, check compatibility:

```sh
python3 tools/cheats/install.py --check
```

Install both cheats:

```sh
python3 tools/cheats/install.py
```

The installer builds and signs `comfort.dylib` locally. It keeps the original game module and changes only the selected module in `native-launch.json`. It backs up the previous launch profile and any existing cheat settings/module under the app data directory.

For another app or data directory, pass `--app` and `--data-root`. Restart ReTuned after installation.

The settings file is:

```text
~/Library/Application Support/Ocarina ReTuned/mods/comfort/settings.json
```

With the fork interface installed, open **Tab → Cheats**. Each switch takes effect during play and saves independently. Instant dialogue applies to the next text page. A failed settings write leaves the previous setting active and displays an error.

With the original upstream interface, edit `infinite_rupees` or `instant_dialogue` in this file and restart. Keep `original_module` unchanged. The installer alone does not add the panel to an existing app.

Disable the wrapper and restore the original module selection:

```sh
python3 tools/cheats/install.py --disable
```

## Build the interface with the Cheats panel

The macOS port and frontend scripts install the panel automatically. The module remains optional. Without it, the panel explains that cheats are unavailable.

Use the same pinned runtime and neutral build process as [the macOS workflow](../.github/workflows/native-arm64.yml). Install its Homebrew dependencies first. From the fork root:

```sh
git clone https://github.com/coccofresco/TriAevum.git engine
git -C engine checkout 17218b0cfdb944645bb5cde5249f313dca7e53ce
python3 tools/native/apply_macos_port.py engine
python3 tools/reframed/apply_native_port.py engine
python3 tools/reframed/apply_renderer_port.py engine
python3 tools/native/apply_audio_port.py engine
python3 tools/frontend/apply_frontend_port.py engine
cmake -S engine -B build/runtime -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 -DCMAKE_PREFIX_PATH=/opt/homebrew \
  -DOOT3D_WHOLE_AOT_PRODUCT_MODE=ON -DOOT3D_DIRECT_AOT_PLUGIN=ON \
  -DOOT3D_REQUIRE_WHOLE_AOT=ON -DOOT3D_REBUILD_WHOLE_AOT=OFF \
  -DOOT3D_REGENERATE_WHOLE_AOT=OFF \
  -DTHREE_DS_RECOMP_ENABLE_IMMERSE=OFF \
  -DTHREE_DS_RECOMP_ENABLE_NRI_UPSCALER_SDKS=OFF \
  -DTHREE_DS_RECOMP_ENABLE_DLSS_MODULE=OFF \
  -DTHREE_DS_RECOMP_ENABLE_SSSR=OFF -DTHREE_DS_RECOMP_ENABLE_CACAO=OFF
cmake --build build/runtime --parallel 3 --target triaevum_public_runtime
```

To retain your installed preparation toolkit in a local test package:

```sh
mkdir -p work/private
cp -R "/Applications/Ocarina ReTuned.app/Contents/Resources/setup" work/private/preparation-kit
python3 tools/native/package_macos.py --engine engine --build build/runtime \
  --minimum-macos 15.0 --private-preparation-kit work/private/preparation-kit \
  --output work/private/Ocarina-ReTuned-Cheats.zip
```

The package helper requires `dylibbundler` and `molten-vk`, as listed in the workflow. It refuses to overwrite an existing output. Keep this test package local. Extract it, retain a backup of your existing app, and replace the app only after closing the game. Your existing prepared game and saves stay in the app data directory.

## Verify

Run the behavior checks without a ROM:

```sh
clang++ -std=c++20 -Wall -Wextra -Werror tests/comfort_cheats.cpp -o /tmp/retuned-cheats-check
/tmp/retuned-cheats-check
python3 -m unittest discover -s tests -p 'test_cheats_install.py' -v
```

The C++ checks cover page boundaries, command-buffer limits, pointer overflow, preserved dialogue choices, and bounded wallet writes. Installer checks cover restoring a launch profile and refusing unrelated module selections.

With a locally built module and your own original prepared module, also check live settings:

```sh
python3 tests/cheat_settings.py --module build/cheats/comfort.dylib \
  --original-module /path/to/your/original/triaevum_title_aot.dylib
```

This check uses a temporary settings directory. It covers independent toggles, persistence across processes, invalid IDs, and failed-write rollback.

## Implementation and credits

The wrapper delegates execution to the existing game module. It adds four filtered block callbacks and preserves the runtime's existing callbacks, execution budget, and memory-write observation ABI.

Dialogue behavior follows the instant-text approach in [OoT3D Randomizer](https://github.com/gamestabled/OoT3D_Randomizer). The command layout and hook locations were checked against the supported prepared image. Third-party notices are included in [tools/cheats/THIRD_PARTY_NOTICES.md](../tools/cheats/THIRD_PARTY_NOTICES.md).

The new module source is licensed under GPL-2.0-or-later. Existing project files retain their licenses and attribution.
