import struct
import tempfile
import unittest
from pathlib import Path

from tools.wii_iso import port_dol, prepare_tree


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

    def test_patch_files_lie_under_the_mods(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            original, root, patch, mod = tmp / "original", tmp / "root", tmp / "patch", tmp / "mod"
            (original / "sys").mkdir(parents=True)
            (original / "files/bin").mkdir(parents=True)
            (original / "files/bin/stage01.bin").write_bytes(b"retail stage")
            (original / "files/bin/stage02.bin").write_bytes(b"retail stage 2")
            (original / "files/Item").mkdir()
            (original / "files/Item/letter.brres").write_bytes(b"retail letter")
            (patch / "bin").mkdir(parents=True)
            (patch / "item").mkdir()
            (patch / "main.dol").write_bytes(b"patch dol")
            (patch / "bin/stage01.bin").write_bytes(b"patch stage")
            (patch / "bin/stage02.bin").write_bytes(b"patch stage 2")
            (patch / "bin/extra.bin").write_bytes(b"not on the disc")
            (patch / "item/letter.brres").write_bytes(b"patch letter")
            (mod / "bin").mkdir(parents=True)
            (mod / "bin/stage02.bin").write_bytes(b"mod stage 2")
            (mod / "bin/new.bin").write_bytes(b"mod adds this")
            (tmp / "mod.dol").write_bytes(b"mod dol")

            self.assertEqual(prepare_tree(original, root, tmp / "mod.dol", mod, patch), 5)
            self.assertEqual((root / "sys/main.dol").read_bytes(), b"mod dol")
            self.assertEqual((root / "files/bin/stage01.bin").read_bytes(), b"patch stage")
            self.assertEqual((root / "files/bin/stage02.bin").read_bytes(), b"mod stage 2")
            self.assertEqual((root / "files/bin/new.bin").read_bytes(), b"mod adds this")
            # the disc ignores case: the patch's item/ is the disc's Item/, not a second folder
            self.assertEqual((root / "files/Item/letter.brres").read_bytes(), b"patch letter")
            self.assertEqual(sorted(p.name for p in (root / "files").iterdir()), ["Item", "bin"])
            # like Riivolution, the patch only replaces what the disc has
            self.assertFalse((root / "files/bin/extra.bin").exists())
            self.assertFalse((root / "files/main.dol").exists())


def make_dol(*sections: bytes) -> bytes:
    """A DOL whose first sections hold `sections`, back to back after the header."""
    offsets, sizes, at = [0] * 18, [0] * 18, 0x100
    for index, data in enumerate(sections):
        offsets[index], sizes[index] = at, len(data)
        at += len(data)
    header = struct.pack(">18I", *offsets) + bytes(72) + struct.pack(">18I", *sizes)
    return header.ljust(0x100, b"\0") + b"".join(sections)


class PortDolTests(unittest.TestCase):
    text = bytes(range(64))
    strings = b"Wii\0" + "ちびロボ！".encode("shift_jis") + b"\0\0" + b"zoom-in-text-xyz" * 3

    def test_changes_follow_the_data_to_its_new_place(self):
        retail = make_dol(self.text, self.strings)
        patched = make_dol(self.text, self.strings.replace("ちびロボ！".encode("shift_jis"), b"Chibi-Robo"))
        # the mod's code is longer, so its strings section starts later and has more in front
        target = make_dol(self.text + b"mod code", b"new!" * 5 + self.strings)
        ported = port_dol(retail, patched, target)
        self.assertIn(b"Wii\0Chibi-Robo\0\0zoom", ported)
        self.assertEqual(len(ported), len(target))
        self.assertEqual(port_dol(retail, patched, retail), patched)

    def test_data_twice_in_retail_is_not_placed_on_the_other_copy(self):
        stretch = b"\0\0\0\0Yes\0\0\0\0\0No\0\0\0"
        retail = make_dol(self.text, b"X" * 40 + stretch + b"Z" * 40 + stretch + b"Q" * 40)
        patched = make_dol(self.text, b"X" * 40 + stretch.replace(b"Yes", b"Oui") + b"Z" * 40 + stretch + b"Q" * 40)
        # the mod reworded the first copy: the second must stay as it is
        target = make_dol(self.text, b"X" * 40 + stretch.replace(b"Yes", b"Yep") + b"Z" * 40 + stretch + b"Q" * 40)
        with self.assertRaises(ValueError):
            port_dol(retail, patched, target)

    def test_ambiguous_or_missing_data_is_an_error(self):
        retail = make_dol(self.text, b"same" * 32)
        patched = make_dol(self.text, b"same" * 16 + b"SAME" + b"same" * 15)
        with self.assertRaises(ValueError):
            port_dol(retail, patched, make_dol(self.text, b"same" * 64))
        with self.assertRaises(ValueError):
            port_dol(retail, patched, make_dol(self.text, b"other" * 16))

    def test_patch_for_another_layout_is_refused(self):
        with self.assertRaises(ValueError):
            port_dol(make_dol(self.text), make_dol(self.text + b"!"), make_dol(self.text))


if __name__ == "__main__":
    unittest.main()
