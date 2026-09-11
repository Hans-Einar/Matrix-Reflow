import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location('register', Path(__file__).resolve().parents[1] / 'xscreensaver/register.py')
register = importlib.util.module_from_spec(spec)
spec.loader.exec_module(register)


class RegistrationTest(unittest.TestCase):
    config = 'mode: one\nselected: 0\nlock: True\nprograms: \\\n  GL: "Old" /opt/xmatrix \\\n  --root \\n\\\n\nother: preserved\n'

    def test_add_idempotent_and_preserves_preferences(self):
        result = register.update(self.config, '/opt/my effects/matrix-reflow')
        self.assertEqual(result, register.update(result, '/opt/my effects/matrix-reflow'))
        self.assertIn("'/opt/my effects/matrix-reflow' --root", result)
        self.assertIn('mode: one\nselected: 0\nlock: True\n', result)
        self.assertIn('GL: "Old" /opt/xmatrix   --root', result)
        self.assertTrue(result.endswith('other: preserved\n'))
        removed = register.update(result, '', True)
        self.assertNotIn('matrix-reflow', removed)
        self.assertEqual(removed, register.update(removed, '', True))

    def test_remove_preserves_selected_effect(self):
        config = 'selected: 2\nprograms: first \\n matrix-reflow --root \\n third\n'
        result = register.update(config, '', True)
        self.assertIn('selected: 1\n', result)
        self.assertIn('third', result)
        self.assertIn('selected: 0\n', register.update(config.replace('selected: 2', 'selected: 1'), '', True))

    def test_missing_resource(self):
        with self.assertRaises(ValueError):
            register.update('lock: True\n', '/opt/matrix-reflow')
        self.assertEqual('lock: True\n', register.update('lock: True\n', '', True))


if __name__ == '__main__':
    unittest.main()
