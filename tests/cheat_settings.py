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
Error = ctypes.CFUNCTYPE(ctypes.c_char_p)


class Api(ctypes.Structure):
    _fields_ = [('version', ctypes.c_uint32), ('size', ctypes.c_uint32),
                ('get', Get), ('set', Set), ('error', Error)]


def load(path):
    library = ctypes.CDLL(str(path))
    query = library.retuned_comfort_query
    query.argtypes = [ctypes.c_uint32]
    query.restype = ctypes.POINTER(Api)
    assert not query(99)
    api = query(1).contents
    assert api.version == 1 and api.size == ctypes.sizeof(Api)
    return library, api


if len(sys.argv) == 3 and sys.argv[1] == '--verify-disabled':
    library, api = load(Path(sys.argv[2]))
    for key in (0, 1):
        value = ctypes.c_bool(True)
        assert api.get(key, ctypes.byref(value)) and not value.value
    raise SystemExit(0)

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--module', type=Path, required=True)
parser.add_argument('--original-module', type=Path, required=True)
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='retuned-cheats-settings-') as temporary:
    target = Path(temporary)
    shutil.copy2(args.module, target / 'comfort.dylib')
    (target / 'settings.json').write_text(json.dumps({
        'original_module': str(args.original_module.resolve()),
        'infinite_rupees': True, 'instant_dialogue': True}))
    library, api = load(target / 'comfort.dylib')
    assert not api.get(0, None)
    original = json.loads((target / 'settings.json').read_text())['original_module']
    for key, field in ((0, 'infinite_rupees'), (1, 'instant_dialogue')):
        assert api.set(key, False)
        value = ctypes.c_bool(True)
        assert api.get(key, ctypes.byref(value)) and not value.value
        stored = json.loads((target / 'settings.json').read_text())
        assert stored[field] is False and stored['original_module'] == original
    subprocess.run([sys.executable, __file__, '--verify-disabled', str(target / 'comfort.dylib')], check=True)
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
print('Passed: independent live toggles, restart persistence, invalid IDs, and save-failure rollback.')
