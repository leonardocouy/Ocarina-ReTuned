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

Set `infinite_rupees` or `instant_dialogue` to `false` and restart to disable an individual cheat. Keep `original_module` unchanged.

Disable the wrapper and restore the original module selection:

```sh
python3 tools/cheats/install.py --disable
```

## Verify

Run the behavior checks without a ROM:

```sh
clang++ -std=c++20 -Wall -Wextra -Werror tests/comfort_cheats.cpp -o /tmp/retuned-cheats-check
/tmp/retuned-cheats-check
python3 -m unittest discover -s tests -p 'test_cheats_install.py' -v
```

The C++ checks cover page boundaries, command-buffer limits, pointer overflow, preserved dialogue choices, and bounded wallet writes. Installer checks cover restoring a launch profile and refusing unrelated module selections.

## Implementation and credits

The wrapper delegates execution to the existing game module. It adds four filtered block callbacks and preserves the runtime's existing callbacks, execution budget, and memory-write observation ABI.

Dialogue behavior follows the instant-text approach in [OoT3D Randomizer](https://github.com/gamestabled/OoT3D_Randomizer). The command layout and hook locations were checked against the supported prepared image. Third-party notices are included in [tools/cheats/THIRD_PARTY_NOTICES.md](../tools/cheats/THIRD_PARTY_NOTICES.md).

The new module source is licensed under GPL-2.0-or-later. Existing project files retain their licenses and attribution.
