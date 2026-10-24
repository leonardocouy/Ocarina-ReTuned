# SPDX-License-Identifier: GPL-2.0-or-later
"""Install, check, or disable cheats without replacing prepared game files."""
import argparse
from datetime import datetime
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

from build import build

RUNTIME_REVISION = '17218b0cfdb944645bb5cde5249f313dca7e53ce'
SUPPORTED_CODE = '6033d96105d18ff666c75e96765e4a88e06e30ed591f1c74d1fc66cc5544fb6c'


def read_json(path):
    return json.loads(path.read_text())


def write_json(path, document):
    with tempfile.NamedTemporaryFile(mode='w', dir=path.parent, delete=False) as stream:
        temporary = Path(stream.name)
        try:
            json.dump(document, stream, indent=2)
            stream.write('\n')
            stream.flush()
            os.fsync(stream.fileno())
        except BaseException:
            temporary.unlink(missing_ok=True)
            raise
    try:
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def plugin_index(profile):
    arguments = profile.get('arguments')
    if (profile.get('format') != 'oot3d_native_game_launch_profile_v1' or
            not isinstance(arguments, list) or not all(isinstance(value, str) for value in arguments)):
        raise ValueError('The game launch profile is invalid.')
    if arguments.count('--title-plugin') != 1:
        raise ValueError('The launch profile must select one game module.')
    index = arguments.index('--title-plugin') + 1
    if index == len(arguments):
        raise ValueError('The launch profile is missing its game module path.')
    return index


def disable(root):
    profile_path = root / 'native-launch.json'
    profile = read_json(profile_path)
    index = plugin_index(profile)
    mod = root / 'mods/comfort'
    settings = read_json(mod / 'settings.json')
    original = Path(settings['original_module'])
    selected = Path(profile['arguments'][index]).resolve()
    if selected not in ((mod / 'comfort.dylib').resolve(), original.resolve()):
        raise ValueError('Another game module is selected. Its launch profile was not changed.')
    if not original.is_file():
        raise ValueError('The original game module is missing. Restore it before disabling cheats.')
    profile['arguments'][index] = str(original)
    write_json(profile_path, profile)


def check(app, root):
    profile = read_json(root / 'native-launch.json')
    index = plugin_index(profile)
    original = Path(profile['arguments'][index]).resolve()
    mod = root / 'mods/comfort'
    if original == (mod / 'comfort.dylib').resolve():
        original = Path(read_json(mod / 'settings.json')['original_module']).resolve()
    arguments = profile['arguments']
    if arguments.count('--a32-process-manifest') != 1:
        raise ValueError('The launch profile must select one prepared game manifest.')
    manifest_index = arguments.index('--a32-process-manifest') + 1
    if manifest_index == len(arguments):
        raise ValueError('The prepared game manifest path is missing.')
    manifest = read_json(Path(arguments[manifest_index]))
    code = Path(manifest['source']['code_bin_path']).resolve()
    if (original.parent != code.parent or not original.is_file() or
            hashlib.sha256(code.read_bytes()).hexdigest() != SUPPORTED_CODE):
        raise ValueError('This prepared game is unsupported. Cheats require the verified 0.1.0 Preview image.')
    executable = app / 'Contents/MacOS/retuned-native'
    result = subprocess.run([str(executable), '--verify-title-plugin', str(original)],
                            check=True, capture_output=True, text=True)
    product = json.loads(result.stdout)
    if product.get('source_commit') != RUNTIME_REVISION or not product.get('private_title_loaded'):
        raise ValueError('The app runtime does not match the supported game-module ABI.')
    return profile, original


def install(app, root):
    profile, original = check(app, root)
    mod = root / 'mods/comfort'
    settings = {'original_module': str(original), 'infinite_rupees': True, 'instant_dialogue': True}
    if (mod / 'settings.json').exists():
        previous = read_json(mod / 'settings.json')
        for key in ('infinite_rupees', 'instant_dialogue'):
            if not isinstance(previous[key], bool):
                raise ValueError('Cheat settings must be true or false.')
            settings[key] = previous[key]
    with tempfile.TemporaryDirectory(prefix='retuned-cheats-build-') as temporary:
        library = build(app, Path(temporary))
        backup = root / 'backups' / ('before-cheats-' + datetime.now().strftime('%Y%m%d-%H%M%S-%f'))
        backup.mkdir(parents=True)
        destinations = [(root / 'native-launch.json', backup / 'native-launch.json'),
                        (mod / 'settings.json', backup / 'settings.json'),
                        (mod / 'comfort.dylib', backup / 'comfort.dylib')]
        for path, saved in destinations:
            if path.exists():
                shutil.copy2(path, saved)
        mod.mkdir(parents=True, exist_ok=True)
        next_library = mod / 'comfort.pending.dylib'
        try:
            shutil.copy2(library, next_library)
            os.replace(next_library, mod / 'comfort.dylib')
            write_json(mod / 'settings.json', settings)
            profile['arguments'][plugin_index(profile)] = str(mod / 'comfort.dylib')
            write_json(root / 'native-launch.json', profile)
        except BaseException:
            for path, saved in destinations:
                if saved.exists():
                    restore = path.with_name(path.name + '.restore')
                    shutil.copy2(saved, restore)
                    os.replace(restore, path)
                else:
                    path.unlink(missing_ok=True)
            raise
        finally:
            next_library.unlink(missing_ok=True)
        shutil.copy2(Path(__file__).with_name('THIRD_PARTY_NOTICES.md'), mod)
    return backup


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', type=Path, default=Path('/Applications/Ocarina ReTuned.app'))
    parser.add_argument('--data-root', type=Path,
                        default=Path.home() / 'Library/Application Support/Ocarina ReTuned')
    actions = parser.add_mutually_exclusive_group()
    actions.add_argument('--check', action='store_true')
    actions.add_argument('--disable', action='store_true')
    args = parser.parse_args()
    app, root = args.app.expanduser().resolve(), args.data_root.expanduser().resolve()
    try:
        if args.disable:
            disable(root)
            print('Cheats disabled. Restart Ocarina ReTuned.')
        elif args.check:
            check(app, root)
            print('The prepared game and runtime support these cheats. No files were changed.')
        else:
            backup = install(app, root)
            print(f'Cheats installed. Restart Ocarina ReTuned. Backup: {backup}')
        return 0
    except (OSError, ValueError, KeyError, subprocess.SubprocessError) as error:
        parser.exit(1, f'Cheats: {error}\n')


if __name__ == '__main__':
    raise SystemExit(main())
