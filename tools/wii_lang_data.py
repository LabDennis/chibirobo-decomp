#!/usr/bin/env python3
"""Build per-language override files for Chibi-Robo! Wii (R24J01) from GC US/PAL discs."""

from __future__ import annotations

import argparse
import difflib
import json
import os
import shutil
import struct
import subprocess
import sys
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Tuple

LANG_SPECS: Dict[str, Tuple[str, str, str]] = {
    # code: (disc_key, gc_bin_sub, gc_tpl_suffix)
    "en": ("us", "e", "us"),
    "de": ("pal", "g", "ge"),
    "fr": ("pal", "f", "fr"),
    "es": ("pal", "s", "sp"),
    "it": ("pal", "i", "it"),
}

# globals.bin data section text tables: (header_off, stride, ptr_offsets, name, count)
GLOBALS_TEXT_TABLES: Tuple[Tuple[int, int, Tuple[int, ...], str, int], ...] = (
    (0x28, 20, (0, 4), "items", 159),
    (0x2C, 4, (0,), "actors", 64),
    (0x30, 12, (0, 4), "attachments", 9),
    (0x34, 4, (0,), "costumes", 9),
    (0x38, 8, (0,), "stages", 30),
    (0x48, 12, (0, 4), "utilibots", 20),
    (0x4C, 12, (0, 4), "stickers", 24),
    (0x50, 8, (0, 4), "status_overview", 13),
)

UNARY_EXPRS = {6, 28, 29, 32, 100, 102, 103, 108, 109, 110, 111, 117, 201, 202, 203}
BINARY_EXPRS = {
    0, 1, 2, 3, 4, 5, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 200,
}
ZERO_EXPRS = {30, 31, 101, 104, 105, 106, 107, 112, 113, 114, 115, 116}


def skip_expr(b: bytes, pos: int, addrs_out: List[int]) -> Tuple[int, Optional[int]]:
    op = b[pos]
    pos += 1
    if op == 23:
        return pos + 2, struct.unpack_from("<h", b, pos)[0]
    if op == 24:
        return pos + 4, struct.unpack_from("<i", b, pos)[0]
    if op == 25:
        addrs_out.append(pos)
        return pos + 4, struct.unpack_from("<I", b, pos)[0]
    if op in (26, 27):
        return pos + 1, None
    if op in UNARY_EXPRS:
        pos, _ = skip_expr(b, pos, addrs_out)
        return pos, None
    if op in BINARY_EXPRS:
        pos, _ = skip_expr(b, pos, addrs_out)
        pos, _ = skip_expr(b, pos, addrs_out)
        return pos, None
    if op == 204:
        pos, _ = skip_expr(b, pos, addrs_out)
        pos, _ = skip_expr(b, pos, addrs_out)
        pos, _ = skip_expr(b, pos, addrs_out)
        return pos, None
    if op in ZERO_EXPRS:
        return pos, None
    raise ValueError(f"Unknown expr op {op} at 0x{pos - 1:x}")


def eval_n(b: bytes, pos: int, n: int, addrs_out: List[int]) -> Tuple[int, List[Optional[int]]]:
    vals: List[Optional[int]] = []
    for _ in range(n):
        pos, v = skip_expr(b, pos, addrs_out)
        vals.append(v)
    return pos, vals


def _decode_non_flow_cmd(b: bytes, cmd: int, pos: int, addr_ofs: List[int]) -> int:
    if cmd == 4:
        n_args = 1 if 15 <= b[pos] <= 22 else 2
        pos, _ = eval_n(b, pos, n_args, addr_ofs)
    elif cmd == 13:
        pos += 2
    elif cmd in (14, 15):
        pass
    elif cmd == 16:
        pos, _ = eval_n(b, pos, 1, addr_ofs)
    elif cmd in (17, 18):
        pos, _ = eval_n(b, pos, 1, addr_ofs)
        while True:
            pos, [a] = eval_n(b, pos, 1, addr_ofs)
            if a == -1:
                break
    elif cmd == 19:
        pos, _ = eval_n(b, pos, 1, addr_ofs)
        while True:
            pos, [a] = eval_n(b, pos, 1, addr_ofs)
            if a == -1:
                break
            pos, _ = eval_n(b, pos, 2, addr_ofs)
    elif cmd == 20:
        pos, _ = eval_n(b, pos, 2, addr_ofs)
    elif cmd == 26:
        pos, _ = eval_n(b, pos, 1, addr_ofs)
    elif cmd == 21:
        pos, _ = eval_n(b, pos, 10, addr_ofs)
    elif cmd == 22:
        length = struct.unpack_from("<H", b, pos)[0]
        end = pos + length
        p = pos + 2
        while p < end:
            p, _ = skip_expr(b, p, addr_ofs)
        if p != end:
            raise ValueError(f"CALL length mismatch at 0x{pos - 1:x}")
        pos = end
    elif cmd == 23:
        pos, [sub] = eval_n(b, pos, 1, addr_ofs)
        if sub == 211 or sub in (240, 243, 251, 252):
            pos, _ = eval_n(b, pos, 4, addr_ofs)
        elif sub in (207, 227):
            pos, _ = eval_n(b, pos, 5, addr_ofs)
        elif sub in (208, 228, 229, 204):
            pos, _ = eval_n(b, pos, 3, addr_ofs)
        elif sub in (220, 236, 237, 238):
            pos, _ = eval_n(b, pos, 1, addr_ofs)
        elif sub == 209:
            pos, _ = eval_n(b, pos, 2, addr_ofs)
        elif sub == 230:
            pass
        elif sub == 232:
            pos, [m2] = eval_n(b, pos, 1, addr_ofs)
            if m2 in (2, 3, 4):
                pos, _ = eval_n(b, pos, 1, addr_ofs)
        else:
            raise ValueError(f"Unknown CAMERA sub {sub} at 0x{pos - 1:x}")
    elif cmd in (24, 46):
        pos, [sub] = eval_n(b, pos, 1, addr_ofs)
        if sub in (200, 204):
            pos, _ = eval_n(b, pos, 2 if sub == 204 else 1, addr_ofs)
        elif sub in (205, 206, 244, 221, 219, 224, 233, 246):
            pos, _ = eval_n(b, pos, 1, addr_ofs)
        elif sub not in (201, 242, 202, 239, 203, 225, 234, 241, 245):
            raise ValueError(f"Unknown WAIT/CHECK sub {sub} at 0x{pos - 1:x}")
    elif cmd == 25:
        pos, _ = eval_n(b, pos, 6, addr_ofs)
    elif cmd == 27:
        pos, _ = eval_n(b, pos, 2, addr_ofs)
    elif cmd == 28:
        pos, [_, mode] = eval_n(b, pos, 2, addr_ofs)
        if mode in (205, 208):
            pos, _ = eval_n(b, pos, 1, addr_ofs)
        elif mode == 207:
            pos, _ = eval_n(b, pos, 2, addr_ofs)
        elif mode != 225:
            raise ValueError(f"Unknown MDIR mode {mode} at 0x{pos - 1:x}")
        pos, _ = eval_n(b, pos, 2, addr_ofs)
    elif cmd == 29:
        pos, _ = eval_n(b, pos, 2, addr_ofs)
    elif cmd == 30:
        pos, _ = eval_n(b, pos, 1, addr_ofs)
    elif cmd == 31:
        pos, [_, sub] = eval_n(b, pos, 2, addr_ofs)
        if sub in (0xDB, 0xCF, 0xE3):
            pos, _ = eval_n(b, pos, 3, addr_ofs)
        else:
            raise ValueError(f"Unknown LIGHT sub {sub} at 0x{pos - 1:x}")
    elif cmd == 32:
        pos, [mid] = eval_n(b, pos, 1, addr_ofs)
        if mid == 1000:
            pos, _ = eval_n(b, pos, 1, addr_ofs)
        elif mid == 1001:
            pos, _ = eval_n(b, pos, 2, addr_ofs)
    elif cmd == 33:
        pos, _ = eval_n(b, pos, 5, addr_ofs)
    elif cmd == 34:
        pos, _ = eval_n(b, pos, 7, addr_ofs)
    elif cmd == 36:
        pos, _ = eval_n(b, pos, 2, addr_ofs)
        saved = (pos, len(addr_ofs))
        try:
            pos, _ = eval_n(b, pos, 2, addr_ofs)
        except ValueError:
            pos = saved[0]
            del addr_ofs[saved[1]:]
    elif cmd == 37:
        pos = b.index(0, pos) + 1
    elif cmd == 38:
        pos, [_, sub] = eval_n(b, pos, 2, addr_ofs)
        if sub == 207:
            pos, _ = eval_n(b, pos, 7, addr_ofs)
        elif sub == 208:
            pos, _ = eval_n(b, pos, 8, addr_ofs)
        elif sub == 220:
            pos, [_, depth] = eval_n(b, pos, 2, addr_ofs)
            if depth is None:
                raise ValueError("dynamic PTCL depth")
            pos, _ = eval_n(b, pos, min(depth, 16), addr_ofs)
        elif sub != 210:
            raise ValueError(f"Unknown PTCL sub {sub} at 0x{pos - 1:x}")
    elif cmd == 39:
        pos, _ = eval_n(b, pos, 3, addr_ofs)
    elif cmd == 40:
        pos, _ = eval_n(b, pos, 4, addr_ofs)
    elif cmd == 41:
        pos, _ = eval_n(b, pos, 5, addr_ofs)
    elif cmd == 42:
        pos, [mode] = eval_n(b, pos, 1, addr_ofs)
        if mode in (201, 242):
            pos, _ = eval_n(b, pos, 9, addr_ofs)
        elif mode == 202:
            pos, _ = eval_n(b, pos, 17, addr_ofs)
        elif mode == 239:
            pos, _ = eval_n(b, pos, 10, addr_ofs)
        elif mode == 226:
            pos, [part] = eval_n(b, pos, 1, addr_ofs)
            if part in (0, 1, 2):
                pos, _ = eval_n(b, pos, 1, addr_ofs)
            elif part == 3:
                pos, _ = eval_n(b, pos, 4, addr_ofs)
            elif part == 4:
                pos, [idx] = eval_n(b, pos, 1, addr_ofs)
                if idx == -3:
                    pos, _ = eval_n(b, pos, 1, addr_ofs)
        elif mode == 234:
            pos, _ = eval_n(b, pos, 5, addr_ofs)
        elif mode == 241:
            pos, _ = eval_n(b, pos, 7, addr_ofs)
        else:
            raise ValueError(f"Unknown SCRN mode {mode} at 0x{pos - 1:x}")
    elif cmd == 44:
        pos, [_, sub] = eval_n(b, pos, 2, addr_ofs)
        if sub in (2, 3):
            pos, _ = eval_n(b, pos, 1, addr_ofs)
        elif sub == 4:
            pos, _ = eval_n(b, pos, 2, addr_ofs)
        elif sub not in (0, 1, 5, 6, 0xF5):
            raise ValueError(f"Unknown SFX sub {sub} at 0x{pos - 1:x}")
    elif cmd == 45:
        pos, _ = eval_n(b, pos, 2, addr_ofs)
    elif cmd == 47:
        pos, _ = eval_n(b, pos, 2, addr_ofs)
    elif cmd == 48:
        pos, [sub] = eval_n(b, pos, 1, addr_ofs)
        if sub == 207:
            pos, _ = eval_n(b, pos, 2, addr_ofs)
        elif sub in (208, 219):
            pos, _ = eval_n(b, pos, 4, addr_ofs)
        elif sub not in (239, 209):
            raise ValueError(f"Unknown WIN sub {sub} at 0x{pos - 1:x}")
    elif cmd == 49:
        pos, [path] = eval_n(b, pos, 1, addr_ofs)
        if path != 0:
            pos, _ = eval_n(b, pos, 5, addr_ofs)
    else:
        raise ValueError(f"Unknown cmd {cmd} at 0x{pos - 1:x}")
    return pos


def parse_script_from(
    b: bytes,
    entry_offsets: Sequence[int],
    base_offset: int = 0,
    follow_run: bool = True,
):
    """Walk reachable CEvt bytecode from entry_offsets.

    Returns (insts, jumps, addr_ofs, msgs) where:
      insts: sorted [(start_off, norm_token, msg_info_or_None, run_target_or_None)]
      jumps: [file_off_of_u32_target]
      addr_ofs: [file_off_of_u32_ADDRESS_OF]
      msgs: [(cmd_off, cmd, next_off_field, msg_start, msg_end)]
    """
    visited = set()
    queue = [o for o in entry_offsets if o != 0]
    insts = []
    jumps: List[int] = []
    addr_ofs: List[int] = []
    msgs = []

    while queue:
        pos = queue.pop()
        while pos not in visited:
            visited.add(pos)
            if pos < 0 or pos >= len(b):
                raise ValueError(f"PC out of bounds: 0x{pos:x}")
            start = pos
            cmd = b[pos]
            pos += 1
            a0 = len(addr_ofs)
            msg = None
            run_tgt = None
            stop = False

            if cmd in (1, 2):
                tok: Tuple[object, ...] = (cmd,)
                stop = True
            elif cmd in (3, 7, 11):
                jumps.append(pos)
                tgt = struct.unpack_from("<I", b, pos)[0]
                if tgt != 0:
                    queue.append(base_offset + tgt)
                tok = (cmd, tgt != 0)
                pos += 4
                stop = True
            elif cmd == 12:
                jumps.append(pos)
                tgt = struct.unpack_from("<I", b, pos)[0]
                if tgt != 0:
                    run_tgt = base_offset + tgt
                    if follow_run:
                        queue.append(run_tgt)
                tok = ("RUN",)
                pos += 4
            elif cmd in (5, 6, 8, 9, 10):
                p0 = pos
                pos, _ = skip_expr(b, pos, addr_ofs)
                expr_key: object = b[p0:pos] if len(addr_ofs) == a0 else (cmd, "addr")
                jumps.append(pos)
                tgt = struct.unpack_from("<I", b, pos)[0]
                if tgt != 0:
                    queue.append(base_offset + tgt)
                tok = (cmd, expr_key)
                pos += 4
            elif cmd in (35, 43):
                jumps.append(pos)
                nxt = struct.unpack_from("<I", b, pos)[0]
                msg_start = pos + 4
                msg_end = base_offset + nxt
                if msg_end < msg_start or msg_end > len(b):
                    raise ValueError(f"Invalid MSG/SELECT next 0x{msg_end:x} at 0x{start:x}")
                tok = ("MSG", cmd)
                msg = (start, cmd, pos, msg_start, msg_end)
                msgs.append(msg)
                pos = msg_end
            else:
                p0 = pos
                pos = _decode_non_flow_cmd(b, cmd, pos, addr_ofs)
                tok = (cmd, b[p0:pos] if len(addr_ofs) == a0 else len(b[p0:pos]))

            insts.append((start, tok, msg, run_tgt))
            if stop:
                break

    insts.sort(key=lambda x: x[0])
    msgs.sort(key=lambda m: m[3])
    return insts, jumps, addr_ofs, msgs


def extract_script_entries(b: bytes, is_globals: bool) -> Tuple[int, Dict[str, int]]:
    entries: Dict[str, int] = {}
    if is_globals:
        scr_off, _ = struct.unpack_from("<2I", b, 16)
        base = 0x18 + scr_off
        i = 0
        first_nz = len(b)
        while base + i * 4 < first_nz:
            val = struct.unpack_from("<I", b, base + i * 4)[0]
            if val != 0:
                first_nz = min(first_nz, base + val)
                entries[f"g{i}"] = base + val
            i += 1
        return base, entries

    hdr = struct.unpack_from("<13I", b, 0)
    so_off = hdr[2]
    idx = 0
    while True:
        val = struct.unpack_from("<I", b, so_off)[0]
        if val == 0xFFFFFFFF:
            break
        if val != 0:
            entries[f"s{idx}"] = val
        idx += 1
        so_off += 4
    for hi in range(3, 9):
        if hdr[hi] != 0:
            entries[f"h{hi}"] = hdr[hi]
    return 0, entries


def align_by_structure(
    bytes_a: bytes,
    bytes_b: bytes,
    is_globals: bool,
    require_same_msg_bytes: bool = False,
) -> Dict[int, Tuple[int, bytes]]:
    """Align messages from bytes_a to bytes_b via entry slots + RUN pairing.

    Returns {msg_start_a: (msg_start_b, msg_bytes_b)} only for unambiguous 1-to-1 matches.
    """
    base_a, ent_a = extract_script_entries(bytes_a, is_globals)
    base_b, ent_b = extract_script_entries(bytes_b, is_globals)
    cache_a: Dict[int, list] = {}
    cache_b: Dict[int, list] = {}

    def get_a(off: int):
        if off not in cache_a:
            cache_a[off] = parse_script_from(bytes_a, [off], base_a, follow_run=False)[0]
        return cache_a[off]

    def get_b(off: int):
        if off not in cache_b:
            cache_b[off] = parse_script_from(bytes_b, [off], base_b, follow_run=False)[0]
        return cache_b[off]

    pair_q: List[Tuple[int, int]] = []
    seen_pairs = set()
    for slot, off_a in ent_a.items():
        if slot in ent_b:
            pair = (off_a, ent_b[slot])
            if pair not in seen_pairs:
                seen_pairs.add(pair)
                pair_q.append(pair)

    msg_cands: Dict[int, set] = {}
    while pair_q:
        oa, ob = pair_q.pop(0)
        ia = get_a(oa)
        ib = get_b(ob)
        ta = [x[1] for x in ia]
        tb = [x[1] for x in ib]
        sm = difflib.SequenceMatcher(None, ta, tb, autojunk=False)
        for tag, i1, i2, j1, j2 in sm.get_opcodes():
            if tag == "equal":
                for k in range(i2 - i1):
                    xa, xb = ia[i1 + k], ib[j1 + k]
                    if xa[2] is not None and xb[2] is not None:
                        ma, mb = xa[2], xb[2]
                        ba = bytes_a[ma[3]:ma[4]]
                        bb = bytes_b[mb[3]:mb[4]]
                        if not require_same_msg_bytes or ba == bb:
                            msg_cands.setdefault(ma[3], set()).add((mb[3], bb))
                    if xa[3] and xb[3]:
                        pair = (xa[3], xb[3])
                        if pair not in seen_pairs:
                            seen_pairs.add(pair)
                            pair_q.append(pair)
            elif tag == "replace":
                ma_list = [ia[i] for i in range(i1, i2) if ia[i][2] is not None]
                mb_list = [ib[j] for j in range(j1, j2) if ib[j][2] is not None]
                if len(ma_list) == len(mb_list) and len(ma_list) > 0:
                    for xa, xb in zip(ma_list, mb_list):
                        ma, mb = xa[2], xb[2]
                        ba = bytes_a[ma[3]:ma[4]]
                        bb = bytes_b[mb[3]:mb[4]]
                        if not require_same_msg_bytes or ba == bb:
                            msg_cands.setdefault(ma[3], set()).add((mb[3], bb))
                ra_list = [ia[i] for i in range(i1, i2) if ia[i][3]]
                rb_list = [ib[j] for j in range(j1, j2) if ib[j][3]]
                if len(ra_list) == len(rb_list) and len(ra_list) > 0:
                    for xa, xb in zip(ra_list, rb_list):
                        pair = (xa[3], xb[3])
                        if pair not in seen_pairs:
                            seen_pairs.add(pair)
                            pair_q.append(pair)

    resolved: Dict[int, Tuple[int, bytes]] = {}
    for moff_a, cands in msg_cands.items():
        texts = {c[1] for c in cands}
        if len(texts) == 1:
            resolved[moff_a] = next(iter(cands))
    return resolved


def relocate_bytecode_region(
    b: bytes,
    entries: Sequence[int],
    base_offset: int,
    replacements: Dict[int, bytes],
    extra_table_offsets: Sequence[int],
) -> bytes:
    """Apply message replacements to b[base_offset:] and relocate all offsets."""
    _, jumps, addr_ofs, msgs = parse_script_from(b, entries, base_offset, follow_run=True)
    msg_spans = []
    for _, _, _, m_start, m_end in msgs:
        if m_start in replacements and replacements[m_start] != b[m_start:m_end]:
            msg_spans.append((m_start, m_end, replacements[m_start]))
    msg_spans.sort(key=lambda x: x[0])

    if not msg_spans:
        return b

    def reloc(old_abs: int) -> int:
        delta = 0
        for s, e, new_b in msg_spans:
            if e <= old_abs:
                delta += len(new_b) - (e - s)
            else:
                break
        return old_abs + delta

    out = bytearray(b)
    # Relocate in-place on `out` BEFORE splicing bytes (offsets are still old offsets)
    for off in extra_table_offsets:
        val = struct.unpack_from("<I", out, off)[0]
        if val != 0:
            struct.pack_into("<I", out, off, reloc(base_offset + val) - base_offset)

    for off in jumps:
        val = struct.unpack_from("<I", out, off)[0]
        if val != 0:
            struct.pack_into("<I", out, off, reloc(base_offset + val) - base_offset)

    for off in addr_ofs:
        val = struct.unpack_from("<I", out, off)[0]
        if val != 0:
            struct.pack_into("<I", out, off, reloc(base_offset + val) - base_offset)

    # Now splice replaced message spans
    rebuilt = bytearray()
    cursor = 0
    for s, e, new_b in msg_spans:
        rebuilt.extend(out[cursor:s])
        rebuilt.extend(new_b)
        cursor = e
    rebuilt.extend(out[cursor:])
    return bytes(rebuilt)


def translate_stage_bin(
    wii_bytes: bytes,
    target_bytes: bytes,
    jp_bytes: Optional[bytes] = None,
) -> Tuple[bytes, Dict[str, object]]:
    _, ent_wii = extract_script_entries(wii_bytes, is_globals=False)
    _, _, _, wii_msgs = parse_script_from(wii_bytes, list(ent_wii.values()), 0, follow_run=True)

    if jp_bytes is not None:
        m_wj = align_by_structure(wii_bytes, jp_bytes, is_globals=False, require_same_msg_bytes=True)
        m_jt = align_by_structure(jp_bytes, target_bytes, is_globals=False, require_same_msg_bytes=False)
        repl: Dict[int, bytes] = {}
        for m_start, (j_start, _) in m_wj.items():
            if j_start in m_jt:
                repl[m_start] = m_jt[j_start][1]
    else:
        m_wt = align_by_structure(wii_bytes, target_bytes, is_globals=False, require_same_msg_bytes=False)
        repl = {m_start: txt for m_start, (_, txt) in m_wt.items()}

    untranslated_indices = [
        idx for idx, (_, _, _, m_start, _) in enumerate(wii_msgs) if m_start not in repl
    ]
    hdr = struct.unpack_from("<13I", wii_bytes, 0)
    table_offs: List[int] = [4 * hi for hi in range(3, 9) if hdr[hi] != 0]
    so_off = hdr[2]
    while True:
        val = struct.unpack_from("<I", wii_bytes, so_off)[0]
        if val == 0xFFFFFFFF:
            break
        if val != 0:
            table_offs.append(so_off)
        so_off += 4

    new_bytes = relocate_bytecode_region(wii_bytes, list(ent_wii.values()), 0, repl, table_offs)
    stats = {
        "total_messages": len(wii_msgs),
        "translated_messages": len(wii_msgs) - len(untranslated_indices),
        "untranslated_messages": len(untranslated_indices),
        "untranslated_indices": untranslated_indices,
    }
    return new_bytes, stats


def _read_cstr(b: bytes, off: int) -> bytes:
    if off == 0:
        return b""
    end = b.index(0, off)
    return b[off:end]


def _extract_globals_table_strings(data_bytes: bytes) -> Dict[Tuple[str, int, int], bytes]:
    out: Dict[Tuple[str, int, int], bytes] = {}
    for hdr_off, stride, ptrs, name, count in GLOBALS_TEXT_TABLES:
        tbl_off = struct.unpack_from("<I", data_bytes, hdr_off)[0]
        if tbl_off == 0:
            continue
        for idx in range(count):
            for p_off in ptrs:
                s_off = struct.unpack_from("<I", data_bytes, tbl_off + idx * stride + p_off)[0]
                if s_off != 0:
                    out[(name, idx, p_off)] = _read_cstr(data_bytes, s_off)
    return out


def translate_globals_bin(
    wii_bytes: bytes,
    target_bytes: bytes,
    jp_bytes: Optional[bytes] = None,
) -> Tuple[bytes, Dict[str, object]]:
    w_hdr = struct.unpack_from("<6I", wii_bytes, 0)
    t_hdr = struct.unpack_from("<6I", target_bytes, 0)
    w_data = bytearray(wii_bytes[0x18 + w_hdr[0] : 0x18 + w_hdr[0] + w_hdr[1]])
    t_data = target_bytes[0x18 + t_hdr[0] : 0x18 + t_hdr[0] + t_hdr[1]]
    j_strs = (
        _extract_globals_table_strings(
            jp_bytes[0x18 + struct.unpack_from("<2I", jp_bytes, 0)[0] :][: struct.unpack_from("<2I", jp_bytes, 0)[1]]
        )
        if jp_bytes is not None
        else None
    )
    t_strs = _extract_globals_table_strings(t_data)

    new_data = bytearray(w_data)
    tbl_translated = 0
    tbl_total = 0
    for hdr_off, stride, ptrs, name, count in GLOBALS_TEXT_TABLES:
        tbl_off = struct.unpack_from("<I", new_data, hdr_off)[0]
        if tbl_off == 0:
            continue
        for idx in range(count):
            for p_off in ptrs:
                ptr_field = tbl_off + idx * stride + p_off
                s_off = struct.unpack_from("<I", new_data, ptr_field)[0]
                if s_off == 0:
                    continue
                w_str = _read_cstr(w_data, s_off)
                tbl_total += 1
                key = (name, idx, p_off)
                if key in t_strs and (j_strs is None or j_strs.get(key) == w_str):
                    chosen = t_strs[key]
                    tbl_translated += 1
                    if chosen != w_str:
                        if len(chosen) <= len(w_str):
                            new_data[s_off : s_off + len(chosen) + 1] = chosen + b"\x00"
                        else:
                            struct.pack_into("<I", new_data, ptr_field, len(new_data))
                            new_data.extend(chosen)
                            new_data.append(0)

    # Translate & relocate scripts section
    _, ent_wii = extract_script_entries(wii_bytes, is_globals=True)
    _, _, _, wii_msgs = parse_script_from(wii_bytes, list(ent_wii.values()), 0x18 + w_hdr[4], follow_run=True)

    if jp_bytes is not None:
        m_wj = align_by_structure(wii_bytes, jp_bytes, is_globals=True, require_same_msg_bytes=True)
        m_jt = align_by_structure(jp_bytes, target_bytes, is_globals=True, require_same_msg_bytes=False)
        repl: Dict[int, bytes] = {}
        for m_start, (j_start, _) in m_wj.items():
            if j_start in m_jt:
                repl[m_start] = m_jt[j_start][1]
    else:
        m_wt = align_by_structure(wii_bytes, target_bytes, is_globals=True, require_same_msg_bytes=False)
        repl = {m_start: txt for m_start, (_, txt) in m_wt.items()}

    untranslated_indices = [
        idx for idx, (_, _, _, m_start, _) in enumerate(wii_msgs) if m_start not in repl
    ]

    scr_base = 0x18 + w_hdr[4]
    table_offs: List[int] = []
    i = 0
    first_nz = len(wii_bytes)
    while scr_base + i * 4 < first_nz:
        val = struct.unpack_from("<I", wii_bytes, scr_base + i * 4)[0]
        if val != 0:
            first_nz = min(first_nz, scr_base + val)
            table_offs.append(scr_base + i * 4)
        i += 1

    relocated_full = relocate_bytecode_region(
        wii_bytes, list(ent_wii.values()), scr_base, repl, table_offs
    )
    new_scripts = relocated_full[scr_base:]
    coll_bytes = wii_bytes[0x18 + w_hdr[2] : 0x18 + w_hdr[2] + w_hdr[3]]

    new_data_len = len(new_data)
    new_coll_off = (new_data_len + 3) & ~3
    new_scr_off = (new_coll_off + len(coll_bytes) + 3) & ~3

    out = bytearray(struct.pack("<6I", 0, new_data_len, new_coll_off, len(coll_bytes), new_scr_off, len(new_scripts)))
    out.extend(new_data)
    out.extend(b"\x00" * (new_coll_off - new_data_len))
    out.extend(coll_bytes)
    out.extend(b"\x00" * (new_scr_off - (new_coll_off + len(coll_bytes))))
    out.extend(new_scripts)

    stats = {
        "total_messages": len(wii_msgs),
        "translated_messages": len(wii_msgs) - len(untranslated_indices),
        "untranslated_messages": len(untranslated_indices),
        "untranslated_indices": untranslated_indices,
        "table_strings_total": tbl_total,
        "table_strings_translated": tbl_translated,
    }
    return bytes(out), stats


def parse_tpl_header(b: bytes) -> List[Tuple[int, int, int]]:
    if len(b) < 12:
        raise ValueError("TPL too short")
    magic, count, tbl_off = struct.unpack_from(">3I", b, 0)
    if magic != 0x0020AF30:
        raise ValueError(f"Bad TPL magic 0x{magic:08x}")
    texs: List[Tuple[int, int, int]] = []
    for i in range(count):
        tex_hdr_off, _ = struct.unpack_from(">2I", b, tbl_off + i * 8)
        h, w, fmt = struct.unpack_from(">2HI", b, tex_hdr_off)
        texs.append((w, h, fmt))
    return texs


def _run_dtk(dtk: Path, args: Sequence[str]) -> str:
    res = subprocess.run([str(dtk), *args], capture_output=True, text=True)
    if res.returncode != 0:
        raise RuntimeError(f"dtk {' '.join(args)} failed: {res.stderr.strip() or res.stdout.strip()}")
    return res.stdout


def _dtk_exists(dtk: Path, vfs_target: str) -> bool:
    res = subprocess.run([str(dtk), "vfs", "ls", vfs_target], capture_output=True, text=True)
    return res.returncode == 0


def _auto_detect_jp_disc(wii_path: Path) -> Optional[Path]:
    # Check <parent_of_R24J01>/GGTJ01_01/*.rvz or *.iso
    jp_dir = wii_path.parent.parent / "GGTJ01_01"
    if not jp_dir.is_dir():
        return None
    for ext in ("*.rvz", "*.iso", "*.gcm"):
        matches = sorted(jp_dir.glob(ext))
        if matches:
            return matches[0]
    return None


def _extract_discs(
    dtk: Path,
    wii_disc: Path,
    us_disc: Path,
    pal_disc: Path,
    jp_disc: Optional[Path],
    work_dir: Path,
) -> Dict[str, Path]:
    paths = {
        "wii_bin": work_dir / "wii" / "bin",
        "wii_tpl": work_dir / "wii" / "tpl",
        "us_bin_e": work_dir / "us" / "bin" / "e",
        "us_tpl": work_dir / "us" / "tpl",
        "pal_bin": work_dir / "pal" / "bin",
        "pal_tpl": work_dir / "pal" / "tpl",
    }
    for p in paths.values():
        if p.exists():
            shutil.rmtree(p)
        p.mkdir(parents=True, exist_ok=True)

    _run_dtk(dtk, ["vfs", "cp", f"{wii_disc}:files/bin", str(paths["wii_bin"])])
    _run_dtk(dtk, ["vfs", "cp", f"{wii_disc}:files/tpl", str(paths["wii_tpl"])])
    _run_dtk(dtk, ["vfs", "cp", f"{us_disc}:files/qp.bin:bin/e", str(paths["us_bin_e"])])
    _run_dtk(dtk, ["vfs", "cp", f"{us_disc}:files/qp.bin:tpl", str(paths["us_tpl"])])
    for sub in ("e", "g", "f", "s", "i"):
        dst = paths["pal_bin"] / sub
        dst.mkdir(parents=True, exist_ok=True)
        _run_dtk(dtk, ["vfs", "cp", f"{pal_disc}:files/qp.bin:bin/{sub}", str(dst)])
    _run_dtk(dtk, ["vfs", "cp", f"{pal_disc}:files/qp.bin:tpl", str(paths["pal_tpl"])])

    if jp_disc is not None:
        jp_bin = work_dir / "jp" / "bin"
        if jp_bin.exists():
            shutil.rmtree(jp_bin)
        jp_bin.mkdir(parents=True, exist_ok=True)
        _run_dtk(dtk, ["vfs", "cp", f"{jp_disc}:files/qp.bin:bin", str(jp_bin)])
        paths["jp_bin"] = jp_bin
    return paths


def _write_depfile(depfile: Path, targets: Sequence[Path], deps: Sequence[Path]) -> None:
    depfile.parent.mkdir(parents=True, exist_ok=True)
    t_str = " ".join(str(t).replace(" ", "\\ ") for t in targets)
    d_str = " ".join(str(d).replace(" ", "\\ ") for d in deps)
    depfile.write_text(f"{t_str}: {d_str}\n", encoding="utf-8")


def _tpl_counterpart_name(wii_name: str, suffix: str) -> str:
    if wii_name == "logo.tpl":
        return f"logo_{suffix}.tpl"
    if wii_name == "title.tpl":
        title_suffix = "en" if suffix == "us" else suffix
        return f"title_{title_suffix}.tpl"
    return wii_name


def build_language_data(
    dtk: Path,
    wii_disc: Path,
    us_disc: Path,
    pal_disc: Path,
    work_dir: Path,
    out_dir: Path,
    jp_disc: Optional[Path] = None,
    depfile: Optional[Path] = None,
) -> Dict[str, object]:
    for label, p in (("dtk", dtk), ("wii", wii_disc), ("us", us_disc), ("pal", pal_disc)):
        if not p.exists():
            raise FileNotFoundError(f"Missing --{label} path: {p}")
    if jp_disc is not None and not jp_disc.exists():
        raise FileNotFoundError(f"Missing --jp path: {jp_disc}")

    if not _dtk_exists(dtk, f"{wii_disc}:files/bin"):
        raise RuntimeError(f"Invalid Wii disc (files/bin missing): {wii_disc}")
    if not _dtk_exists(dtk, f"{us_disc}:files/qp.bin"):
        raise RuntimeError(f"Invalid US GC disc (files/qp.bin missing): {us_disc}")
    if not _dtk_exists(dtk, f"{pal_disc}:files/qp.bin"):
        raise RuntimeError(f"Invalid PAL GC disc (files/qp.bin missing): {pal_disc}")

    extracted = _extract_discs(dtk, wii_disc, us_disc, pal_disc, jp_disc, work_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    wii_fnt_empty = _dtk_exists(dtk, f"{wii_disc}:files/fnt") and not _run_dtk(
        dtk, ["vfs", "ls", f"{wii_disc}:files/fnt"]
    ).strip()

    # Record Wii-only asset directories whose container format differs from GC (.brres/.thp vs .dat)
    wii_asset_dirs = []
    for d in ("Item", "how_to_chibirobo", "chibihouse", "kitchen", "living", "movie", "hbm"):
        if _dtk_exists(dtk, f"{wii_disc}:files/{d}"):
            wii_asset_dirs.append(d)

    wii_bins = sorted(extracted["wii_bin"].glob("*.bin"))
    wii_tpls = sorted(extracted["wii_tpl"].glob("*.tpl"))

    report: Dict[str, object] = {
        "jp_reference_used": jp_disc is not None,
        "font": {
            "status": "no_disc_font",
            "wii_fnt_empty": wii_fnt_empty,
            "pal_disc_font": False,
            "note": "PAL/US 512x512 I4 single-byte font is compiled into main.dol (CFont::Init); R24J01 uses ROM font.",
        },
        "non_transferable_asset_dirs": wii_asset_dirs,
        "languages": {},
    }

    written_files: List[Path] = []
    for code, (disc_key, bin_sub, tpl_suffix) in LANG_SPECS.items():
        lang_out = out_dir / "lang" / code
        if lang_out.exists():
            shutil.rmtree(lang_out)

        tgt_bin_dir = extracted["us_bin_e"] if disc_key == "us" else (extracted["pal_bin"] / bin_sub)
        tgt_tpl_dir = extracted["us_tpl"] if disc_key == "us" else extracted["pal_tpl"]
        jp_bin_dir = extracted.get("jp_bin")

        transferred_files: List[str] = []
        identical_files: List[str] = []
        non_transferable_files: List[Dict[str, object]] = []
        wii_only_files: List[str] = []
        total_msgs = 0
        translated_msgs = 0
        untranslated_msgs = 0
        per_bin_details: Dict[str, object] = {}

        for wii_bin_path in wii_bins:
            name = wii_bin_path.name
            rel_path = f"bin/{name}"
            wii_b = wii_bin_path.read_bytes()
            tgt_path = tgt_bin_dir / name
            if not tgt_path.exists():
                wii_only_files.append(rel_path)
                continue
            tgt_b = tgt_path.read_bytes()
            jp_b = (jp_bin_dir / name).read_bytes() if (jp_bin_dir and (jp_bin_dir / name).exists()) else None

            if name == "globals.bin":
                new_b, stats = translate_globals_bin(wii_b, tgt_b, jp_b)
                # Verify rebuilt globals.bin parses cleanly
                _, ent_chk = extract_script_entries(new_b, is_globals=True)
                scr_off = struct.unpack_from("<I", new_b, 16)[0]
                parse_script_from(new_b, list(ent_chk.values()), 0x18 + scr_off, follow_run=True)
            else:
                new_b, stats = translate_stage_bin(wii_b, tgt_b, jp_b)
                _, ent_chk = extract_script_entries(new_b, is_globals=False)
                parse_script_from(new_b, list(ent_chk.values()), 0, follow_run=True)

            total_msgs += int(stats["total_messages"])
            translated_msgs += int(stats["translated_messages"])
            untranslated_msgs += int(stats["untranslated_messages"])
            if stats["untranslated_messages"] > 0 or name == "globals.bin":
                per_bin_details[rel_path] = stats

            if new_b != wii_b:
                dst = lang_out / "bin" / name
                dst.parent.mkdir(parents=True, exist_ok=True)
                dst.write_bytes(new_b)
                written_files.append(dst)
                transferred_files.append(rel_path)
            else:
                identical_files.append(rel_path)

        for wii_tpl_path in wii_tpls:
            name = wii_tpl_path.name
            rel_path = f"tpl/{name}"
            tgt_name = _tpl_counterpart_name(name, tpl_suffix)
            tgt_tpl_path = tgt_tpl_dir / tgt_name
            if not tgt_tpl_path.exists():
                wii_only_files.append(rel_path)
                continue
            wii_b = wii_tpl_path.read_bytes()
            tgt_b = tgt_tpl_path.read_bytes()
            if len(wii_b) != len(tgt_b) or parse_tpl_header(wii_b) != parse_tpl_header(tgt_b):
                non_transferable_files.append({"path": rel_path, "reason": "format_or_size_mismatch"})
                continue
            if wii_b == tgt_b:
                identical_files.append(rel_path)
                continue
            dst = lang_out / "tpl" / name
            dst.parent.mkdir(parents=True, exist_ok=True)
            dst.write_bytes(tgt_b)
            written_files.append(dst)
            transferred_files.append(rel_path)

        report["languages"][code] = {
            "transferred_files": transferred_files,
            "identical_files": identical_files,
            "non_transferable_files": non_transferable_files,
            "wii_only_files": wii_only_files,
            "total_messages": total_msgs,
            "translated_messages": translated_msgs,
            "untranslated_messages": untranslated_msgs,
            "bin_details": per_bin_details,
        }

    report_path = out_dir / "report.json"
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if depfile is not None:
        deps = [wii_disc, us_disc, pal_disc]
        if jp_disc is not None:
            deps.append(jp_disc)
        _write_depfile(depfile, [report_path, *written_files], deps)
    return report


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(description="Generate Wii multi-language override files from GC discs.")
    parser.add_argument("--dtk", required=True, type=Path)
    parser.add_argument("--wii", required=True, type=Path)
    parser.add_argument("--us", required=True, type=Path)
    parser.add_argument("--pal", required=True, type=Path)
    parser.add_argument("--jp", default=None, type=Path, help="Optional GC JP disc (auto-detected if sibling GGTJ01_01 exists)")
    parser.add_argument("--work", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--depfile", default=None, type=Path)
    args = parser.parse_args(argv)

    jp_disc = args.jp if args.jp is not None else _auto_detect_jp_disc(args.wii)
    try:
        build_language_data(
            dtk=args.dtk,
            wii_disc=args.wii,
            us_disc=args.us,
            pal_disc=args.pal,
            work_dir=args.work,
            out_dir=args.out,
            jp_disc=jp_disc,
            depfile=args.depfile,
        )
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
