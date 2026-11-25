#!/usr/bin/env python3
"""Put retail's ASCII '0' padding back into a linked DOL.

python3 tools/fill_padding.py <main.dol> <main.elf.MAP> <main_link_inputs.json>

Our linker pads with zeros, retail used 0x30, so any unit linked from source
breaks the hash without this. Retail fills:
  - alignment gaps between objects, based on the next input's ELF section alignment
  - alignment gaps between symbols inside compiled objects with a padding rule
    in configure.py
      small_sections     eligible sections 0x80 bytes or smaller
      small_data         .sdata and .sdata2
      small_data+data    plus .data
      small_data+rodata  plus .rodata
  - alignment gaps between exception tables in compiled objects with any rule

PAL doesn't do any of this, config.fill_padding turns it off there. Only
zero bytes in our own DOL get touched, nothing comes from the retail one.
"""
import json
import re
import struct
import sys
from pathlib import Path

# sections where retail shows the fill, add more only with evidence
SECTIONS = {".text", ".rodata", ".data", ".sdata", ".sdata2", "extab"}
FILL = 0x30
# small_sections: biggest section that still gets 0x30 inside
SMALL_SECTION = 0x80
SMALL_DATA = {".sdata", ".sdata2"}


def section_alignments(path: Path) -> dict[str, int]:
    data = path.read_bytes()
    if data[:6] != b"\x7fELF\x01\x02":
        sys.exit(f"fill_padding: expected a 32-bit big-endian ELF object: {path}")

    table_offset, = struct.unpack_from(">I", data, 32)
    entry_size, count, names_index = struct.unpack_from(">HHH", data, 46)
    section_header = struct.Struct(">10I")
    headers = [
        section_header.unpack_from(data, table_offset + i * entry_size)
        for i in range(count)
    ]
    names_offset = headers[names_index][4]
    alignments = {}
    for name_offset, _, flags, _, _, _, _, _, alignment, _ in headers:
        if flags & 2:  # SHF_ALLOC
            start = names_offset + name_offset
            name = data[start : data.index(b"\0", start)].decode("ascii")
            alignments[name] = alignment
    return alignments


def is_alignment_gap(start: int, end: int, alignment: int) -> bool:
    return (
        0 < end - start < 0x20
        and alignment > 1
        and (start + alignment - 1) // alignment * alignment == end
    )


def fills_inside(rule: str, section: str, size: int) -> bool:
    if section == "extab":
        return True
    if rule == "small_sections":
        return size <= SMALL_SECTION
    if rule == "small_data":
        return section in SMALL_DATA
    if rule == "small_data+data":
        return section in SMALL_DATA or section == ".data"
    if rule == "small_data+rodata":
        return section in SMALL_DATA or section == ".rodata"
    sys.exit(f"fill_padding: unknown padding rule {rule}")


RE_LAYOUT = re.compile(r"^(\S+) section layout$")
RE_ENTRY = re.compile(r"^  [0-9a-f]{8} ([0-9a-f]{6,8}) ([0-9a-f]{8}) +(\d+) (\S+) \t(\S+)")


def object_ranges(map_text: str):
    """Return object ranges and their symbols' ranges and alignments by section."""
    out = {}
    current = None
    for line in map_text.splitlines():
        m = RE_LAYOUT.match(line)
        if m:
            current = m.group(1)
            continue
        if current not in SECTIONS:
            continue
        m = RE_ENTRY.match(line)
        if not m:
            continue
        size, addr, alignment, name, obj = m.groups()
        size, addr, alignment = int(size, 16), int(addr, 16), int(alignment)
        # an object's section entry carries the section name as its symbol,
        # its symbols follow it
        if name == current:
            out.setdefault(current, []).append((addr, addr + size, obj, []))
        elif out.get(current) and out[current][-1][2] == obj and size:
            out[current][-1][3].append((addr, addr + size, alignment))
    return out


def main() -> None:
    dol_path, map_path = Path(sys.argv[1]), Path(sys.argv[2])
    link_inputs = json.loads(Path(sys.argv[3]).read_text())
    padding_rules = {Path(path).name: rule for path, rule in link_inputs.items() if rule}
    alignments = {Path(path).name: section_alignments(Path(path)) for path in link_inputs}
    dol = bytearray(dol_path.read_bytes())
    offs = struct.unpack(">18I", dol[0:72])
    addrs = struct.unpack(">18I", dol[72:144])
    sizes = struct.unpack(">18I", dol[144:216])

    filled = 0

    def fill(start: int, end: int, what: str) -> None:
        nonlocal filled
        for offset, address, size in zip(offs, addrs, sizes):
            if address <= start < end <= address + size:
                file_start = offset + start - address
                file_end = offset + end - address
                if file_end <= len(dol):
                    break
        else:
            sys.exit(
                f"fill_padding: {what} at 0x{start:08X}..0x{end:08X} "
                "does not fit in a file-backed DOL section"
            )
        gap = dol[file_start:file_end]
        if any(gap):
            sys.exit(f"fill_padding: nonzero padding {what} at 0x{start:08X}")
        dol[file_start:file_end] = bytes([FILL]) * len(gap)
        filled += 1

    for section, ranges in object_ranges(map_path.read_text(errors="replace")).items():
        ranges.sort()
        # rule 1, between objects. the linker only pads up to the next object's alignment.
        # The map gives source objects' sizes before dead stripping. Keep the
        # declared extent of extracted objects, including unnamed tail bytes.
        for (_, end, obj, syms), (start, _, next_obj, _) in zip(ranges, ranges[1:]):
            if syms and obj in padding_rules:
                end = max(e for _, e, _ in syms)
            if is_alignment_gap(end, start, alignments[next_obj][section]):
                fill(end, start, f"after {obj} in {section}")
        # rule 2, inside objects built from source
        for start, end, obj, syms in ranges:
            rule = padding_rules.get(obj)
            if not syms or not rule or not fills_inside(rule, section, end - start):
                continue
            # At a shared address, the most aligned alias constrains the gap.
            syms.sort(key=lambda symbol: (symbol[0], -symbol[2]))
            covered_end = syms[0][1]
            for sym_start, sym_end, alignment in syms[1:]:
                if is_alignment_gap(covered_end, sym_start, alignment):
                    fill(covered_end, sym_start, f"inside {obj} {section}")
                # An alias can start inside a larger symbol and end before it.
                covered_end = max(covered_end, sym_end)
    dol_path.write_bytes(dol)
    if filled:
        print(f"fill_padding: filled {filled} gap(s)")


if __name__ == "__main__":
    main()
