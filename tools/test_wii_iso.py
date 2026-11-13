import tempfile
import unittest
from pathlib import Path

from tools.wii_iso import prepare_tree


class PrepareTreeTests(unittest.TestCase):
    def test_mod_files_replace_links_and_original_stays(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            original, root, mod = tmp / "original", tmp / "root", tmp / "mod"
            (original / "sys").mkdir(parents=True)
            (original / "files/bin").mkdir(parents=True)
            (original / "sys/main.dol").write_bytes(b"retail dol")
            (original / "files/bin/stage01.bin").write_bytes(b"retail stage")
            (original / "files/bin/globals.bin").write_bytes(b"retail globals")
            (mod / "files/bin").mkdir(parents=True)
            (mod / "main.dol").write_bytes(b"mod dol")
            (mod / "files/bin/globals.bin").write_bytes(b"mod globals")

            self.assertEqual(prepare_tree(original, root, mod / "main.dol", mod / "files"), 2)
            self.assertEqual((root / "sys/main.dol").read_bytes(), b"mod dol")
            self.assertEqual((root / "files/bin/globals.bin").read_bytes(), b"mod globals")
            self.assertEqual((root / "files/bin/stage01.bin").read_bytes(), b"retail stage")
            self.assertEqual((original / "sys/main.dol").read_bytes(), b"retail dol")
            self.assertEqual((original / "files/bin/globals.bin").read_bytes(), b"retail globals")

            # a file the mod no longer has comes back from the original on the next build
            (mod / "files/bin/globals.bin").unlink()
            self.assertEqual(prepare_tree(original, root, mod / "main.dol", mod / "files"), 1)
            self.assertEqual((root / "files/bin/globals.bin").read_bytes(), b"retail globals")


if __name__ == "__main__":
    unittest.main()
