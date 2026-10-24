import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

INSTALL = Path(__file__).resolve().parents[1] / 'tools/cheats/install.py'


class CheatInstallationTests(unittest.TestCase):
    def prepare(self, root, selected):
        mod = root / 'mods/comfort'
        mod.mkdir(parents=True)
        original = root / 'installation/title.dylib'
        original.parent.mkdir()
        original.write_text('local test fixture')
        settings = {'original_module': str(original), 'infinite_rupees': True, 'instant_dialogue': False}
        (mod / 'settings.json').write_text(json.dumps(settings))
        profile = {'format': 'oot3d_native_game_launch_profile_v1',
                   'arguments': ['--title-plugin', str(selected(mod, original)), '--save-data', str(root / 'saves')]}
        (root / 'native-launch.json').write_text(json.dumps(profile))
        return mod, original, profile

    def run_disable(self, root):
        return subprocess.run([sys.executable, str(INSTALL), '--disable', '--data-root', str(root)],
                              capture_output=True, text=True)

    def test_disable_preserves_save_location_and_cheat_preferences(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            mod, original, profile = self.prepare(root, lambda mod, original: mod / 'comfort.dylib')
            previous_settings = (mod / 'settings.json').read_bytes()
            result = self.run_disable(root)
            self.assertEqual(result.returncode, 0, result.stderr)
            profile['arguments'][1] = str(original)
            self.assertEqual(json.loads((root / 'native-launch.json').read_text()), profile)
            self.assertEqual((mod / 'settings.json').read_bytes(), previous_settings)
            self.assertEqual(self.run_disable(root).returncode, 0)

    def test_disable_refuses_to_replace_another_module(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.prepare(root, lambda mod, original: root / 'another-module.dylib')
            previous = (root / 'native-launch.json').read_bytes()
            result = self.run_disable(root)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('Another game module is selected', result.stderr)
            self.assertEqual((root / 'native-launch.json').read_bytes(), previous)

    def test_disable_keeps_profile_when_original_module_is_missing(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            _, original, _ = self.prepare(root, lambda mod, original: mod / 'comfort.dylib')
            original.unlink()
            previous = (root / 'native-launch.json').read_bytes()
            result = self.run_disable(root)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('original game module is missing', result.stderr)
            self.assertEqual((root / 'native-launch.json').read_bytes(), previous)


if __name__ == '__main__':
    unittest.main()
