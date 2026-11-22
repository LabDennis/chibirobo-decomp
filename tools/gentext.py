#!/usr/bin/env python3
"""Write the game's displayed text tables as C initializer lines, read from the disc.

The text is not kept in the repository: for one edition this reads
orig/<V>/sys/main.dol, follows the pointers of each table named in TABLES
(address and size from config/<V>/symbols.txt) and writes
build/<V>/include/assets/text/<symbol>.inc, one '"...",' line per entry, which
src/game/stage_data.cpp includes inside the table's initializer braces.
A file is only rewritten when its content changes, so ninja does not rebuild.

Usage: gentext.py <version> [--root <repo>]
"""

import argparse
import re
import struct
import sys
from pathlib import Path

# char* tables of displayed text; an edition generates those its symbols.txt names
TABLES = [
    f"g{lang}{kind}"
    for lang in ("English", "German", "French", "Spanish", "Italian", "Japanese")
    for kind in ("InterfaceText", "PCDialogue")
]

HEX = b"0123456789abcdefABCDEF"
ESCAPES = {ord("\\"): "\\\\", ord('"'): '\\"', ord("\n"): "\\n", ord("\t"): "\\t"}


def dol_sections(dol: bytes):
    """(address, size, file offset) of the 7 text and 11 data sections."""
    offs = struct.unpack_from(">18I", dol, 0x00)
    addrs = struct.unpack_from(">18I", dol, 0x48)
    sizes = struct.unpack_from(">18I", dol, 0x90)
    return [(a, s, o) for o, a, s in zip(offs, addrs, sizes) if s]


def reader(dol: bytes):
    sections = dol_sections(dol)

    def at(addr: int) -> int:
        for a, s, o in sections:
            if a <= addr < a + s:
                return o + addr - a
        raise ValueError(f"0x{addr:08X} is not in main.dol")

    return at


def quote(text: bytes) -> str:
    """C string literal as the source writes it: \\xNN for non-ASCII bytes, a
    hex escape never followed by a hex digit ("\\x83" "b")."""
    out = ""
    after_hex = False
    for c in text:
        if after_hex and c in HEX:
            out += '" "'
        after_hex = False
        if c in ESCAPES:
            out += ESCAPES[c]
        elif 0x20 <= c < 0x7F:
            out += chr(c)
        else:
            out += f"\\x{c:02X}"
            after_hex = True
    return f'"{out}"'


def symbols(path: Path):
    pat = re.compile(r"^(\w+) = \.\w+:0x([0-9A-F]+); // type:object size:0x([0-9A-F]+)")
    found = {}
    for line in path.read_text().splitlines():
        m = pat.match(line)
        if m and m.group(1) in TABLES:
            found[m.group(1)] = (int(m.group(2), 16), int(m.group(3), 16))
    return found


def table_lines(dol: bytes, at, addr: int, size: int):
    lines = []
    for i in range(size // 4):
        (ptr,) = struct.unpack_from(">I", dol, at(addr + 4 * i))
        start = at(ptr)
        lines.append("\t" + quote(dol[start : dol.index(b"\0", start)]) + ",\n")
    return "".join(lines)


def tables(version: str, root: Path):
    """{output path: (address, size)} of the tables this edition has."""
    out_dir = root / "build" / version / "include" / "assets" / "text"
    found = symbols(root / "config" / version / "symbols.txt")
    return {out_dir / f"{name}.inc": found[name] for name in found}


def table_outputs(version: str, root: Path):
    """The .inc files gentext writes for this edition (for configure.py)."""
    return list(tables(version, root))


def generate(version: str, root: Path):
    dol = (root / "orig" / version / "sys" / "main.dol").read_bytes()
    at = reader(dol)
    written = []
    for out, (addr, size) in tables(version, root).items():
        assert size % 4 == 0, f"{out.stem}: size 0x{size:X} is not a pointer table"
        out.parent.mkdir(parents=True, exist_ok=True)
        text = table_lines(dol, at, addr, size)
        if not out.exists() or out.read_text(encoding="ascii") != text:
            out.write_text(text, encoding="ascii")
        written.append(out)
    return written


def _selftest():
    assert quote(b"\x83b\x83V") == '"\\x83" "b\\x83V"'
    assert quote(b'a\\b\n\t"') == '"a\\\\b\\n\\t\\""'
    assert quote(b"\x81@") == '"\\x81@"'


if __name__ == "__main__":
    _selftest()
    p = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    p.add_argument("version")
    p.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    args = p.parse_args()
    for f in generate(args.version, args.root):
        print(f, file=sys.stderr)
