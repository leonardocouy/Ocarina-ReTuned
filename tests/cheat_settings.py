# SPDX-License-Identifier: GPL-2.0-or-later
"""Check live settings using a locally built wrapper and original game module."""
import argparse
from pathlib import Path
import ctypes
import json
import shutil
import subprocess
import sys
import tempfile


Get = ctypes.CFUNCTYPE(ctypes.c_bool, ctypes.c_uint32, ctypes.POINTER(ctypes.c_bool))
Set = ctypes.CFUNCTYPE(ctypes.c_bool, ctypes.c_uint32, ctypes.c_bool)
Request = ctypes.CFUNCTYPE(ctypes.c_bool, ctypes.c_uint32, ctypes.c_uint32)
Error = ctypes.CFUNCTYPE(ctypes.c_char_p)
Notice = ctypes.CFUNCTYPE(ctypes.c_char_p, ctypes.POINTER(ctypes.c_uint64))
Skulltula = ctypes.CFUNCTYPE(ctypes.c_bool, ctypes.POINTER(ctypes.c_float), ctypes.POINTER(ctypes.c_bool))
FIELDS = (
    'infinite_rupees', 'instant_dialogue', 'levitate', 'infinite_bombs',
    'infinite_nuts', 'infinite_seeds', 'infinite_sticks', 'infinite_arrows',
    'infinite_beans', 'freeze_timers', 'run_fast', 'skulltula_detector', 'sinking_lure')


Fishing = ctypes.CFUNCTYPE(ctypes.c_bool, ctypes.POINTER(ctypes.c_float), ctypes.POINTER(ctypes.c_bool))


class Api(ctypes.Structure):
    _fields_ = [('version', ctypes.c_uint32), ('size', ctypes.c_uint32),
                ('get', Get), ('set', Set), ('request', Request), ('error', Error), ('notice', Notice), ('skulltula', Skulltula), ('fishing', Fishing)]


def load(path):
    library = ctypes.CDLL(str(path))
    query = library.retuned_comfort_query
    query.argtypes = [ctypes.c_uint32]
    query.restype = ctypes.POINTER(Api)
    assert not query(99)
    api = query(6).contents
    assert api.version == 6 and api.size == ctypes.sizeof(Api)
    return library, api


if len(sys.argv) == 4 and sys.argv[1] == '--verify-state':
    library, api = load(Path(sys.argv[2]))
    for key in range(len(FIELDS)):
        value = ctypes.c_bool(True)
        assert api.get(key, ctypes.byref(value)) and value.value == (sys.argv[3][key] == '1')
    raise SystemExit(0)

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--module', type=Path, required=True)
parser.add_argument('--original-module', type=Path, required=True)
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='retuned-cheats-settings-') as temporary:
    target = Path(temporary)
    shutil.copy2(args.module, target / 'comfort.dylib')
    (target / 'settings.json').write_text(json.dumps({
        'original_module': str(args.original_module.resolve())}))
    library, api = load(target / 'comfort.dylib')
    assert not api.get(0, None)
    assert not api.fishing(None, None)
    assert not api.skulltula(None, None)
    assert not api.skulltula(ctypes.byref(ctypes.c_float()), ctypes.byref(ctypes.c_bool()))
    def restart_with(expected):
        subprocess.run([sys.executable, __file__, '--verify-state', str(target / 'comfort.dylib'), expected], check=True)
    for key in range(len(FIELDS)):
        value = ctypes.c_bool(True)
        assert api.get(key, ctypes.byref(value)) and not value.value
    restart_with('0' * len(FIELDS))
    original = str(args.original_module.resolve())
    for key, field in enumerate(FIELDS):
        assert api.set(key, True)
        assert api.get(key, ctypes.byref(value)) and value.value
        stored = json.loads((target / 'settings.json').read_text())
        assert stored[field] is True and stored['original_module'] == original
    restart_with('1' * len(FIELDS))
    for key in (1, 3, 5, 7):
        assert api.set(key, False)
    restart_with('1010101011111')
    for key in range(len(FIELDS)):
        assert api.set(key, False)
    restart_with('0' * len(FIELDS))
    serial = ctypes.c_uint64(0)
    assert not api.notice(None)
    assert api.set(2, True)
    assert api.notice(ctypes.byref(serial)) == b'Levitation ON'
    assert not api.notice(ctypes.byref(serial))
    assert api.set(2, False)
    assert api.notice(ctypes.byref(serial)) == b'Levitation OFF'
    assert not api.request(5, 31)
    assert api.request(5, 0)
    assert api.request(5, 30)
    assert not api.request(99, 0x14)
    assert not api.request(0, 0xFFFF)
    assert not api.request(1, 0xFF)
    assert api.request(0, 0x5800)
    assert api.request(1, 0x18)
    assert not api.set(99, True)
    assert not api.get(99, ctypes.byref(ctypes.c_bool()))
    (target / 'settings.pending.json').mkdir()
    assert not api.set(0, True)
    assert b'Could not save cheats' in api.error()
    value = ctypes.c_bool(True)
    assert api.get(0, ctypes.byref(value)) and not value.value
    assert json.loads((target / 'settings.json').read_text())['infinite_rupees'] is False
    assert api.set(0, True)
    assert api.get(0, ctypes.byref(value)) and value.value
    restart_with('1' + '0' * (len(FIELDS) - 1))
print('Passed: all cheats default off, every toggle persists, mixed states survive restart, and failed writes preserve state.')
