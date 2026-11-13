"""Synthetic retail-style metadata, with no game text or other disc assets."""

import struct
import os
import hashlib
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools.wii_stage_names import build_overlay, write_overlay


def local_file(names=None, marker=b"W"):
    names = names or {}
    data = bytearray(84)
    for slot in range(21):
        if slot in (15, 16, 17):
            continue
        start = len(data)
        struct.pack_into("<I", data, slot * 4, start)
        if slot not in (11, 13, 14):
            data.extend(marker + bytes([slot]))
            continue
        count = {11: 64, 13: 9, 14: 30}[slot]
        fields = 2 if slot == 14 else 1
        data.extend(bytes(count * fields * 4))
        for field in range(fields):
            for index in range(count):
                raw = names.get((slot, index, field), f"Field{slot}-{index}-{field}".encode())
                pointer = start + 4 * (fields * index + field)
                struct.pack_into("<I", data, pointer, len(data))
                data.extend(raw + b"\0")
    collision = marker + b"coll"
    script = marker + b"script"
    data_pad = bytes((-len(data)) % 4)
    collision_pad = bytes((-len(collision)) % 4)
    collision_pos = len(data) + len(data_pad)
    script_pos = collision_pos + len(collision) + len(collision_pad)
    header = struct.pack("<6I", 0, len(data), collision_pos, len(collision), script_pos, len(script))
    return header + data + data_pad + collision + collision_pad + script + b"\xfa"


def word(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def cell(blob, slot, index, field=0):
    return 24 + word(blob, 24 + slot * 4) + 4 * ((2 if slot == 14 else 1) * index + field)


def value(blob, slot, index, field=0):
    start = 24 + word(blob, cell(blob, slot, index, field))
    return blob[start:blob.index(b"\0", start)]


class WiiStageNamesTest(unittest.TestCase):
    def test_write_overlay_protects_inputs_and_existing_output(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            wii, gc, output = root / "wii.bin", root / "gc.bin", root / "out.bin"
            base = local_file()
            wii.write_bytes(base)
            donor = local_file({(11, 0, 0): b"Changed"})
            gc.write_bytes(donor)
            output.write_bytes(b"previous output")
            for alias in (wii, gc):
                with self.assertRaisesRegex(ValueError, "different from both inputs"):
                    write_overlay(gc, wii, alias)
            with self.assertRaisesRegex(ValueError, "original R24J01"):
                write_overlay(gc, wii, output)
            self.assertEqual(output.read_bytes(), b"previous output")
            # Only the private integration test can use the genuine target. Pin
            # this synthetic file's digest to exercise the same guarded writer.
            with patch("tools.wii_stage_names.R24J01_GLOBALS_SHA256", hashlib.sha256(base).hexdigest()):
                nested = root / "files" / "bin" / "globals.bin"
                write_overlay(gc, wii, nested)
                self.assertEqual(value(nested.read_bytes(), 11, 0), b"Changed")
                gc.write_bytes(b"truncated")
                with self.assertRaisesRegex(ValueError, "truncated"):
                    write_overlay(gc, wii, output)
                self.assertEqual(output.read_bytes(), b"previous output")
                gc.write_bytes(donor)
                with patch("tools.wii_stage_names.os.replace", side_effect=OSError("replace failed")):
                    with self.assertRaisesRegex(OSError, "replace failed"):
                        write_overlay(gc, wii, output)
                self.assertEqual({p.name for p in root.iterdir()}, {"wii.bin", "gc.bin", "out.bin", "files"})
                self.assertEqual(output.read_bytes(), b"previous output")
            self.assertEqual(wii.read_bytes(), base)
            self.assertEqual(gc.read_bytes(), donor)

    def test_output_cannot_be_a_hardlink_to_input(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            wii, gc, alias = root / "wii.bin", root / "gc.bin", root / "alias.bin"
            wii.write_bytes(local_file())
            gc.write_bytes(local_file(marker=b"G"))
            try:
                os.link(gc, alias)
            except OSError as exc:
                self.skipTest(f"hard links unavailable: {exc}")
            with self.assertRaisesRegex(ValueError, "different from both inputs"):
                write_overlay(gc, wii, alias)

    def test_names_only(self):
        base = local_file()
        gc = local_file({(11, 7, 0): b"New actor", (13, 4, 0): b"New costume",
                         (14, 29, 0): b"New stage", (14, 29, 1): b"GC-only description"}, b"G")
        out, report = build_overlay(base, gc)
        changed = [(11, 7), (13, 4), (14, 29)]
        self.assertEqual(report["changed"], ["actor[7]", "costume[4]", "stage[29]"])
        for slot, index in changed:
            self.assertEqual(value(out, slot, index), value(gc, slot, index))
        self.assertEqual(value(out, 14, 29, 1), value(base, 14, 29, 1))
        # Compare every original metadata byte except the three changed offset words.
        original_data_length = word(base, 4)
        restored = bytearray(out[24:24 + original_data_length])
        for slot, index in changed:
            pos = cell(base, slot, index)
            self.assertGreaterEqual(word(out, pos), original_data_length)
            restored[pos - 24:pos - 20] = base[pos:pos + 4]
        self.assertEqual(restored, base[24:24 + original_data_length])
        self.assertEqual(out[24 + word(out, 8):], base[24 + word(base, 8):])
        self.assertEqual(word(out, 8) % 4, 0)
        self.assertEqual(word(out, 16) - word(out, 8), word(base, 16) - word(base, 8))
        self.assertEqual(out, build_overlay(base, gc)[0])

    def test_empty_unsupported_and_length(self):
        base = local_file()
        gc = local_file({(11, 0, 0): b"", (11, 1, 0): b"bad\n", (11, 2, 0): b"\\C88888888",
                         (11, 3, 0): b"prixe\x81", (11, 4, 0): b"$", (11, 5, 0): b"a" * 128,
                         (11, 6, 0): b"a" * 127, (13, 8, 0): b"M\xe9lodie",
                         (14, 0, 0): b"x" * 21, (14, 1, 0): b"x" * 20})
        out, report = build_overlay(base, gc)
        self.assertEqual(report["changed"], ["actor[6]", "stage[1]"])
        self.assertEqual(report["kept for empty donor"], ["actor[0]"])
        self.assertEqual(report["kept for length"], ["actor[5]", "stage[0]"])
        self.assertEqual(len(report["kept for unsupported characters"]), 5)
        for i in range(6):
            self.assertEqual(value(out, 11, i), value(base, 11, i))
        folded, report = build_overlay(base, gc, fold=True)
        self.assertEqual(report["folded"], ["costume[8]"])
        self.assertEqual(value(folded, 13, 8), b"Melodie")
        self.assertEqual(len(report["kept for unsupported characters"]), 4)

    def test_unchanged_donor_preserves_whole_file(self):
        base = local_file({(11, 1, 0): b"\x83\x41"})
        self.assertEqual(build_overlay(base, base)[0], base)

    def test_invalid_donor(self):
        base = local_file()
        invalid = []
        for pos, replacement in [(4, 0xfffffff0), (12, 0xfffffff0), (16, 3),
                                 (24 + 11 * 4, len(base)), (24 + 14 * 4, 0),
                                 (cell(base, 11, 0), word(base, cell(base, 11, 0)) + 4),
                                 (cell(base, 14, 1, 1), word(base, cell(base, 14, 0, 1)))]:
            copy = bytearray(base)
            struct.pack_into("<I", copy, pos, replacement)
            invalid.append(copy)
        copy = bytearray(base)
        copy[24 + word(base, 24 + 12 * 4) - 1] = 0x41  # final actor terminator
        invalid.extend((copy, base[:23], base[:-12], base + bytes(4)))
        for i, corrupt in enumerate(invalid):
            with self.subTest(case=i), self.assertRaises(ValueError):
                build_overlay(base, corrupt)


if __name__ == "__main__":
    unittest.main()
