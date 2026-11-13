import tempfile
import unittest
from pathlib import Path

from tools.wii_lang_install import install


class LanguageInstallTests(unittest.TestCase):
    def test_replaces_only_generated_languages(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            shared = root / "shared"
            for code in ("en", "de", "fr", "es", "it"):
                (shared / "lang" / code).mkdir(parents=True)
            target = root / "mod/files/lang"
            target.mkdir(parents=True)
            sibling = target.parent / "opening.bnr"
            sibling.write_bytes(b"banner")
            (target / "stale").touch()
            (shared / "lang/de/test").write_bytes(b"asset")
            install(shared, target)
            self.assertEqual((target / "de/test").read_bytes(), b"asset")
            self.assertFalse((target / "stale").exists())
            self.assertEqual(sibling.read_bytes(), b"banner")
            (shared / "lang/en").rmdir()
            with self.assertRaises(ValueError):
                install(shared, target)
            self.assertTrue((target / "de/test").is_file())

    def test_fixed_language_installs_only_its_own(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for code in ("en", "de", "fr", "es", "it"):
                (root / "shared/lang" / code).mkdir(parents=True)
                (root / "shared/lang" / code / "bin").write_bytes(code.encode())
            target = root / "mod/files/lang"
            install(root / "shared", target, ("de",))
            self.assertEqual(sorted(p.name for p in target.iterdir()), ["de"])
            self.assertEqual((target / "de/bin").read_bytes(), b"de")
