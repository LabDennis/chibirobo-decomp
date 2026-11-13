"""Build a private Wii globals.bin overlay from an extracted GameCube globals.bin."""

import argparse
import hashlib
import os
import struct
import tempfile
import unicodedata
from collections import defaultdict
from pathlib import Path


HEADER = struct.Struct("<6I")
DIRECTORY = struct.Struct("<21I")
WORD = struct.Struct("<I")
R24J01_GLOBALS_SHA256 = "9b868ead42ba15d502f34d03f946e5c726fd3d782f22ff6c3087e702746e510d"
# Directory byte offset, row count, number of pointer fields per row.
TABLES = {"actor": (0x2C, 64, 1), "costume": (0x34, 9, 1), "stage": (0x38, 30, 2)}


def metadata(blob):
    if len(blob) < HEADER.size:
        raise ValueError("truncated globals.bin header")
    data, size, collision, collision_size, script, script_size = HEADER.unpack_from(blob)
    end = len(blob) - HEADER.size
    if (data != 0 or size < DIRECTORY.size or collision != ((size + 3) & ~3)
            or script != ((collision + collision_size + 3) & ~3) or not collision_size or not script_size
            or not 0 <= end - script - script_size <= 3):
        raise ValueError("invalid globals.bin section offsets or lengths")
    return blob[HEADER.size:HEADER.size + size]


def name_tables(data):
    """Validate retail row counts and string extents, including stage descriptions."""
    offsets = DIRECTORY.unpack_from(data)
    if offsets[0] != DIRECTORY.size or any(p and not DIRECTORY.size <= p < len(data) for p in offsets):
        raise ValueError("invalid metadata directory")
    starts = [p for p in offsets if p]
    if len(set(starts)) != len(starts):
        raise ValueError("overlapping metadata tables")

    tables = {}
    for name, (slot, count, fields) in TABLES.items():
        start = offsets[slot // 4]
        limit = min((p for p in starts if p > start), default=len(data))
        stride = fields * WORD.size
        cursor = start + count * stride
        if not start or cursor > limit:
            raise ValueError(f"invalid {name} table size")
        rows = []
        # Retail puts every name directly after the records, then every description.
        for field in range(fields):
            for index in range(count):
                pos = start + index * stride + field * WORD.size
                if WORD.unpack_from(data, pos)[0] != cursor:
                    raise ValueError(f"{name} {index}: unexpected field {field} string offset")
                end = data.find(b"\0", cursor, limit)
                if end < 0:
                    raise ValueError(f"{name} {index}: unterminated field {field}")
                if field == 0:
                    rows.append((pos, data[cursor:end]))
                cursor = end + 1
        if cursor != limit:
            raise ValueError(f"{name} strings do not end at the next metadata table")
        tables[name] = rows
    return tables


# Letters and punctuation without a useful ASCII canonical decomposition.
ASCII_EQUIVALENTS = str.maketrans({
    "ß": "ss", "Æ": "AE", "æ": "ae", "Œ": "OE", "œ": "oe", "Ø": "O", "ø": "o",
    "‘": "'", "’": "'", "“": '"', "”": '"', "–": "-", "—": "-", "…": "...",
    "¡": "!", "¿": "?", "\N{NO-BREAK SPACE}": " ",
})


def western_name(raw, fold):
    try:
        text = raw.decode("cp1252" if fold else "ascii")
    except UnicodeDecodeError:
        return None
    if fold:
        text = unicodedata.normalize("NFD", text.translate(ASCII_EQUIVALENTS))
        text = "".join(c for c in text if not unicodedata.combining(c))
    # Backslash introduces a graphics escape, and the message formatter rewrites '$'.
    if any(not " " <= c <= "~" or c in "\\$" for c in text):
        return None
    return text.encode("ascii")


def build_overlay(wii, gc, fold=False):
    data = metadata(wii)
    original = name_tables(data)
    donor = name_tables(metadata(gc))
    updated = bytearray(data)
    report = defaultdict(list)
    for table in TABLES:
        for index, ((pos, old), (_, raw)) in enumerate(zip(original[table], donor[table])):
            key = f"{table}[{index}]"
            if raw == old:
                report["unchanged"].append(key)
                continue
            if not raw:
                report["kept for empty donor"].append(key)
                continue
            name = western_name(raw, fold)
            if name is None:
                report["kept for unsupported characters"].append(key)
                continue
            # R24J01 stage titles display 20 glyphs. The message formatter has a
            # 128-byte string scratch buffer for arguments such as actor names.
            if len(name) > (20 if table == "stage" else 127):
                report["kept for length"].append(key)
                continue
            if name != raw:
                report["folded"].append(key)
            if name == old:
                report["unchanged"].append(key)
                continue
            WORD.pack_into(updated, pos, len(updated))
            updated.extend(name + b"\0")
            report["changed"].append(key)

    if len(updated) == len(data):
        return wii, report
    _, _, collision, collision_size, script, script_size = HEADER.unpack_from(wii)
    new_collision = (len(updated) + 3) & ~3
    new_script = script + new_collision - collision
    header = HEADER.pack(0, len(updated), new_collision, collision_size, new_script, script_size)
    padding = bytes(new_collision - len(updated))
    return header + updated + padding + wii[HEADER.size + collision:], report


def write_overlay(gc_file, wii_file, output, fold=False):
    for source in (gc_file, wii_file):
        if output.resolve() == source.resolve() or (output.exists() and output.samefile(source)):
            raise ValueError("choose an output path different from both inputs")
    wii = wii_file.read_bytes()
    if hashlib.sha256(wii).hexdigest() != R24J01_GLOBALS_SHA256:
        raise ValueError("expected the original R24J01 files/bin/globals.bin as Wii input")
    result, report = build_overlay(wii, gc_file.read_bytes(), fold)
    output.parent.mkdir(parents=True, exist_ok=True)
    tmp = None
    try:
        with tempfile.NamedTemporaryFile(dir=output.parent, delete=False) as f:
            tmp = Path(f.name)
            f.write(result)
        os.replace(tmp, output)
    finally:
        if tmp is not None:
            tmp.unlink(missing_ok=True)
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("gc", type=Path, help="private, extracted GameCube language globals.bin")
    parser.add_argument("wii", type=Path, help="original R24J01 files/bin/globals.bin")
    parser.add_argument("output", type=Path, help="overlay file, e.g. build/wii-en-US/files/bin/globals.bin")
    parser.add_argument("--fold-cp1252", action="store_true",
                        help="allow lossy conversion of Western accents and punctuation to ASCII")
    args = parser.parse_args()
    try:
        report = write_overlay(args.gc, args.wii, args.output, args.fold_cp1252)
    except (ValueError, OSError) as exc:
        parser.exit(1, f"{parser.prog}: {exc}\n")
    print(f"{args.output}: {len(report['changed'])} of 103 display names changed")
    for reason in ("kept for empty donor", "kept for unsupported characters", "kept for length", "folded"):
        if report[reason]:
            print(f"{reason} ({len(report[reason])}): {', '.join(report[reason])}")


if __name__ == "__main__":
    main()
