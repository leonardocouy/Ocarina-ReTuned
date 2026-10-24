# SPDX-License-Identifier: GPL-2.0-or-later
"""Build the local cheat module against an installed Mac preparation toolkit."""
import argparse
from pathlib import Path
import platform
import subprocess


def build(app, output):
    if platform.system() != 'Darwin' or platform.machine() != 'arm64':
        raise ValueError('Cheats currently support Apple silicon Macs only.')
    source = Path(__file__).resolve().parent
    kit = app / 'Contents/Resources/setup'
    runtime = kit / 'engine/tools/oot3d'
    includes = [runtime / 'native_game_runtime', runtime / 'native_a32_runtime',
                runtime / 'native_a32_runtime/upstream', kit / 'headers']
    for path in includes:
        if not path.is_dir():
            raise ValueError('The app is missing preparation headers: ' + path.name)
    output.mkdir(parents=True, exist_ok=True)
    library = output / 'comfort.dylib'
    subprocess.run(['clang++', '-std=c++20', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-dynamiclib', '-fvisibility=hidden', '-arch', 'arm64',
                    *[arg for path in includes for arg in ('-isystem', str(path))],
                    str(source / 'module.cpp'), '-o', str(library)], check=True)
    subprocess.run(['codesign', '--sign', '-', str(library)], check=True)
    return library


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', type=Path, default=Path('/Applications/Ocarina ReTuned.app'))
    parser.add_argument('--output', type=Path, default=Path('build/cheats'))
    args = parser.parse_args()
    print(build(args.app.resolve(), args.output.resolve()))
