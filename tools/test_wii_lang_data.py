#!/usr/bin/env python3
"""Unit tests for tools/wii_lang_data.py using synthetic scripts and files."""

from __future__ import annotations

import struct
import unittest

from tools.wii_lang_data import (
    align_by_structure,
    extract_script_entries,
    parse_script_from,
    parse_tpl_header,
    translate_globals_bin,
    translate_stage_bin,
)


def _imm16(val: int) -> bytes:
    return struct.pack("<Bh", 23, val)


def _addr_of(off: int) -> bytes:
    return struct.pack("<BI", 25, off)


def _build_stage_bin(scripts_builder) -> bytes:
    """Build a minimal synthetic StageFileHeader + scriptOffsets + script body."""
    hdr_size = 13 * 4
    so_off = hdr_size
    # Build body starting after 3 scriptOffsets entries + 0xFFFFFFFF terminator (16 bytes)
    body_start = so_off + 16
    body, entries, setup_off = scripts_builder(body_start)
    hdr = [0] * 13
    hdr[2] = so_off
    hdr[3] = setup_off
    so_bytes = struct.pack("<4I", entries[0], entries[1], entries[2], 0xFFFFFFFF)
    return struct.pack("<13I", *hdr) + so_bytes + body


def _make_two_slot_script(
    msg0: bytes,
    msg1: bytes,
    base: int = 0,
    extra_cmd_in_s0: bytes = b"",
    include_s2: bool = False,
):
    """Create a synthetic script region with IF, MSG, ADDRESS_OF, RUN, GOTO, RETURN."""
    # Layout:
    # s0: IF (IMM16 1) -> skip_tgt; MSG msg0; SET(addr_of(after_s0_data)); GOTO s1; [after_s0_data: RETURN]
    # s1: MSG msg1; RETURN
    # s2 (optional): MSG b"EXTRA"; RETURN
    buf = bytearray()

    def rel(pos: int) -> int:
        return base + pos

    s0_off = rel(len(buf))
    buf.extend(extra_cmd_in_s0)
    # IF (IMM16 1) -> skip_tgt
    buf.append(5)
    buf.extend(_imm16(1))
    if_tgt_field = len(buf)
    buf.extend(b"\x00\x00\x00\x00")
    # MSG msg0
    buf.append(35)
    msg0_nxt_field = len(buf)
    buf.extend(b"\x00\x00\x00\x00")
    buf.extend(msg0)
    msg0_end = rel(len(buf))
    struct.pack_into("<I", buf, msg0_nxt_field, msg0_end)
    # SET (IMM16 0) (ADDRESS_OF ret_label)
    buf.append(4)
    buf.extend(_imm16(0))
    addr_field = len(buf) + 1  # +1 skips op 25
    buf.extend(_addr_of(0))
    # GOTO s1
    buf.append(3)
    goto_field = len(buf)
    buf.extend(b"\x00\x00\x00\x00")
    # ret_label (target of IF and ADDRESS_OF)
    ret_label = rel(len(buf))
    buf.append(2)  # RETURN
    struct.pack_into("<I", buf, if_tgt_field, ret_label)
    struct.pack_into("<I", buf, addr_field, ret_label)

    s1_off = rel(len(buf))
    struct.pack_into("<I", buf, goto_field, s1_off)
    buf.append(35)
    msg1_nxt_field = len(buf)
    buf.extend(b"\x00\x00\x00\x00")
    buf.extend(msg1)
    struct.pack_into("<I", buf, msg1_nxt_field, rel(len(buf)))
    buf.append(2)  # RETURN

    s2_off = 0
    if include_s2:
        s2_off = rel(len(buf))
        buf.append(35)
        nxt2 = len(buf)
        buf.extend(b"\x00\x00\x00\x00")
        buf.extend(b"ONLY_WII")
        struct.pack_into("<I", buf, nxt2, rel(len(buf)))
        buf.append(2)

    return bytes(buf), (s0_off, s1_off, s2_off), s1_off


class TestWiiLangData(unittest.TestCase):
    def test_relocate_offsets_on_longer_and_shorter_messages(self) -> None:
        wii_bin = _build_stage_bin(lambda b: _make_two_slot_script(b"JP0", b"JP_LONG_ONE", base=b))
        tgt_bin = _build_stage_bin(
            lambda b: _make_two_slot_script(b"MUCH_LONGER_ENGLISH_0", b"S1", base=b)
        )

        out_bin, stats = translate_stage_bin(wii_bin, tgt_bin, jp_bytes=wii_bin)
        self.assertEqual(stats["total_messages"], 2)
        self.assertEqual(stats["translated_messages"], 2)
        self.assertEqual(stats["untranslated_messages"], 0)

        # Re-parse relocated binary and check all offsets & messages
        _, ent = extract_script_entries(out_bin, is_globals=False)
        _, jumps, addr_ofs, msgs = parse_script_from(out_bin, list(ent.values()), 0)
        self.assertEqual([out_bin[m[3]:m[4]] for m in msgs], [b"MUCH_LONGER_ENGLISH_0", b"S1"])

        # s1 shifted by (+18 - 9) = +9? Let's verify exact delta from wii_bin:
        delta0 = len(b"MUCH_LONGER_ENGLISH_0") - len(b"JP0")
        hdr_wii = struct.unpack_from("<13I", wii_bin, 0)
        hdr_out = struct.unpack_from("<13I", out_bin, 0)
        self.assertEqual(hdr_out[3], hdr_wii[3] + delta0)
        # ADDRESS_OF target (ret_label) sits after msg0, so it also shifted by delta0
        self.assertEqual(len(addr_ofs), 1)
        addr_val = struct.unpack_from("<I", out_bin, addr_ofs[0])[0]
        self.assertEqual(out_bin[addr_val], 2)  # points to RETURN

    def test_jp_match_required_and_ambiguous_fallback(self) -> None:
        wii_bin = _build_stage_bin(lambda b: _make_two_slot_script(b"WII_SAME", b"WII_CHANGED", base=b))
        jp_bin = _build_stage_bin(lambda b: _make_two_slot_script(b"WII_SAME", b"GC_JP_OLD", base=b))
        tgt_bin = _build_stage_bin(lambda b: _make_two_slot_script(b"TRANS_0", b"TRANS_1", base=b))

        out_bin, stats = translate_stage_bin(wii_bin, tgt_bin, jp_bytes=jp_bin)
        self.assertEqual(stats["translated_messages"], 1)
        self.assertEqual(stats["untranslated_messages"], 1)
        self.assertEqual(stats["untranslated_indices"], [1])

        _, ent = extract_script_entries(out_bin, is_globals=False)
        _, _, _, msgs = parse_script_from(out_bin, list(ent.values()), 0)
        self.assertEqual([out_bin[m[3]:m[4]] for m in msgs], [b"TRANS_0", b"WII_CHANGED"])

    def test_ambiguous_run_conflict_stays_japanese(self) -> None:
        # Two slots s0 and s1 both RUN the same shared subroutine, but in target_bin
        # s0 and s1 RUN two different subroutines with different texts -> ambiguous!
        def build_wii(base: int):
            buf = bytearray()
            sub_off = base + 12  # after s0 (6 bytes) and s1 (6 bytes)
            # s0: RUN sub; RETURN
            buf.extend(struct.pack("<BI", 12, sub_off) + b"\x02")
            # s1: RUN sub; RETURN
            buf.extend(struct.pack("<BI", 12, sub_off) + b"\x02")
            # sub: MSG "SHARED"; RETURN
            buf.append(35)
            nxt = len(buf)
            buf.extend(b"\x00\x00\x00\x00SHARED\x02")
            struct.pack_into("<I", buf, nxt, base + len(buf) - 1)
            return bytes(buf), (base, base + 6, 0), 0

        def build_tgt(base: int):
            buf = bytearray()
            sub0_off = base + 12
            # s0: RUN sub0; RETURN
            buf.extend(struct.pack("<BI", 12, sub0_off) + b"\x02")
            # s1: RUN sub1; RETURN (placeholder)
            buf.extend(struct.pack("<BI", 12, 0) + b"\x02")
            # sub0: MSG "OPT_A"; RETURN
            buf.append(35)
            n0 = len(buf)
            buf.extend(b"\x00\x00\x00\x00OPT_A\x02")
            struct.pack_into("<I", buf, n0, base + len(buf) - 1)
            # sub1: MSG "OPT_B"; RETURN
            sub1_off = base + len(buf)
            struct.pack_into("<I", buf, 7, sub1_off)
            buf.append(35)
            n1 = len(buf)
            buf.extend(b"\x00\x00\x00\x00OPT_B\x02")
            struct.pack_into("<I", buf, n1, base + len(buf) - 1)
            return bytes(buf), (base, base + 6, 0), 0

        wii_bin = _build_stage_bin(build_wii)
        tgt_bin = _build_stage_bin(build_tgt)
        # align_by_structure must reject conflicting candidates OPT_A vs OPT_B
        self.assertEqual(align_by_structure(wii_bin, tgt_bin, is_globals=False), {})
        out_bin, stats = translate_stage_bin(wii_bin, tgt_bin)
        self.assertEqual(stats["translated_messages"], 0)
        self.assertEqual(out_bin, wii_bin)

    def test_missing_counterpart_fallback(self) -> None:
        wii_bin = _build_stage_bin(
            lambda b: _make_two_slot_script(b"A", b"B", base=b, include_s2=True)
        )
        tgt_bin = _build_stage_bin(
            lambda b: _make_two_slot_script(b"EN_A", b"EN_B", base=b, include_s2=False)
        )
        out_bin, stats = translate_stage_bin(wii_bin, tgt_bin)
        self.assertEqual(stats["total_messages"], 3)
        self.assertEqual(stats["translated_messages"], 2)
        self.assertEqual(stats["untranslated_indices"], [2])
        _, ent = extract_script_entries(out_bin, is_globals=False)
        _, _, _, msgs = parse_script_from(out_bin, list(ent.values()), 0)
        self.assertEqual([out_bin[m[3]:m[4]] for m in msgs], [b"EN_A", b"EN_B", b"ONLY_WII"])

    def test_globals_bin_tables_and_scripts(self) -> None:
        def make_globals(item_name: bytes, msg_bytes: bytes) -> bytes:
            # data: 0x60 header (only 0x28 points to items table at 0x60; 159*20 bytes), then string
            data = bytearray(0x60 + 159 * 20)
            struct.pack_into("<I", data, 0x28, 0x60)
            struct.pack_into("<I", data, 0x60, len(data))
            data.extend(item_name + b"\x00")
            coll = b"\xAA\xBB\xCC\xDD"
            # scripts: 4-byte offset table [4], then MSG + RETURN
            scr = bytearray(struct.pack("<I", 4))
            scr.append(35)
            nxt = len(scr)
            scr.extend(b"\x00\x00\x00\x00")
            scr.extend(msg_bytes)
            struct.pack_into("<I", scr, nxt, len(scr))
            scr.append(2)

            d_pad = (len(data) + 3) & ~3
            c_off = d_pad
            s_off = (c_off + len(coll) + 3) & ~3
            hdr = struct.pack("<6I", 0, len(data), c_off, len(coll), s_off, len(scr))
            return (
                hdr
                + data
                + b"\x00" * (d_pad - len(data))
                + coll
                + b"\x00" * (s_off - (c_off + len(coll)))
                + scr
            )

        wii_g = make_globals(b"JP_IT", b"JP_SCR")
        tgt_g = make_globals(b"VERY_LONG_ENGLISH_ITEM", b"LONG_ENGLISH_SCRIPT_MSG")
        out_g, stats = translate_globals_bin(wii_g, tgt_g, jp_bytes=wii_g)
        self.assertEqual(stats["table_strings_translated"], 1)
        self.assertEqual(stats["translated_messages"], 1)

        hdr = struct.unpack_from("<6I", out_g, 0)
        out_data = out_g[0x18 + hdr[0] : 0x18 + hdr[0] + hdr[1]]
        str_off = struct.unpack_from("<I", out_data, 0x60)[0]
        self.assertTrue(out_data[str_off:].startswith(b"VERY_LONG_ENGLISH_ITEM\x00"))
        out_coll = out_g[0x18 + hdr[2] : 0x18 + hdr[2] + hdr[3]]
        self.assertEqual(out_coll, b"\xAA\xBB\xCC\xDD")
        _, ent = extract_script_entries(out_g, is_globals=True)
        _, _, _, msgs = parse_script_from(out_g, list(ent.values()), 0x18 + hdr[4])
        self.assertEqual([out_g[m[3]:m[4]] for m in msgs], [b"LONG_ENGLISH_SCRIPT_MSG"])

    def test_tpl_header_and_validation(self) -> None:
        # 1 texture: 64x32, fmt=5
        tpl = struct.pack(">3I2I2HI", 0x0020AF30, 1, 12, 20, 0, 32, 64, 5)
        self.assertEqual(parse_tpl_header(tpl), [(64, 32, 5)])
        with self.assertRaises(ValueError):
            parse_tpl_header(b"\x00" * 12)


if __name__ == "__main__":
    unittest.main()
