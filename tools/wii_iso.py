"""Build a Wii disc image from the original disc and a mod's main.dol and files.

dtk extracts the original once; every build then links a fresh tree from it, puts the mod's files in, and has
Wiimms ISO Tools (wit) write and encrypt the image. Ticket and TMD come out fake-signed, which Dolphin accepts.

A translation patch made for the retail disc (its main.dol and file folders, as Riivolution loads them) can come
along: its files go onto the disc as they are, and its changes to main.dol move over to the mod's main.dol.
"""

import argparse
import os
import shutil
import struct
import subprocess
from pathlib import Path
from typing import Optional


def overlay_files(folder: Path) -> list:
    """The files a folder puts onto the disc's files/, a main.dol at its top excepted."""
    if not folder.is_dir():
        return []
    return sorted(p for p in folder.rglob("*") if p.is_file() and p != folder / "main.dol")


def disc_path(files: Path, relative: Path) -> Path:
    """`relative` under `files`, spelled the way the disc spells each part it already has: the disc's file system
    ignores case, so a patch's item/ is the disc's Item/."""
    at = files
    for part in relative.parts:
        match = None
        if at.is_dir():
            match = next((entry for entry in at.iterdir() if entry.name.lower() == part.lower()), None)
        at = match or at / part
    return at


def prepare_tree(original: Path, root: Path, dol: Path, files: Path, patch: Optional[Path] = None) -> int:
    """Link `root` from `original`, then replace sys/main.dol and lay `patch` and then the mod's `files` over files/;
    the original stays untouched. Like Riivolution, the patch only replaces files the disc has and the mod may add
    new ones. Returns the number of files replaced or added."""
    if root.exists():
        shutil.rmtree(root)
    shutil.copytree(original, root, copy_function=os.link)
    replaced = {root / "sys/main.dol": dol}
    for folder, may_add in ((patch, False), (files, True)):
        for source in overlay_files(folder) if folder else []:
            target = disc_path(root / "files", source.relative_to(folder))
            if may_add or target.is_file():
                replaced[target] = source
            else:
                print(f"skipped {source}: the disc has no such file")
    for target, source in replaced.items():
        # unlink first: target is a hard link into the original tree
        if target.exists():
            target.unlink()
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
    return len(replaced)


def dol_sections(dol: bytes) -> list:
    """(file offset, size) of each of a DOL's 18 sections; the header holds offsets, then addresses, then sizes."""
    offsets = struct.unpack(">18I", dol[0:72])
    sizes = struct.unpack(">18I", dol[144:216])
    return list(zip(offsets, sizes))


def port_dol(retail: bytes, patched: bytes, target: bytes) -> bytes:
    """Carry the changes `patched` makes to `retail` over to `target`, a main.dol built differently from retail.

    Each changed stretch is found in the same section of `target` by its retail bytes plus as much unchanged
    retail context as it takes to make the match unique, never fewer than 16 bytes in all. Raises ValueError for a
    change it cannot place, so the image never gets a half-ported main.dol."""
    if retail[:0x100] != patched[:0x100]:
        raise ValueError("the patch's main.dol has another layout than the disc's: it was made for another release")
    sections = dol_sections(retail)
    target_sections = dol_sections(target)
    out = bytearray(target)
    for index, (offset, size) in enumerate(sections):
        end = offset + size
        changes = []
        at = offset
        while at < end:
            if retail[at] == patched[at]:
                at += 1
                continue
            start = at
            while at < end and (retail[at] != patched[at] or retail[at : at + 8] != patched[at : at + 8]):
                at += 1
            changes.append((start, at - start))
        target_start, target_size = target_sections[index]
        section = target[target_start : target_start + target_size]
        for start, length in changes:
            place = _unique_place(retail, offset, end, start, length, section)
            if place is None:
                raise ValueError(f"cannot place the patch's change at main.dol offset 0x{start:x} in the mod's main.dol")
            out[target_start + place : target_start + place + length] = patched[start : start + length]
    return bytes(out)


def _unique_place(retail: bytes, begin: int, end: int, start: int, length: int, section: bytes):
    """Where retail[start:start+length] sits in `section`, judged by the least context that occurs once in both
    the retail section and `section`; context found once in the mod but twice in retail could be the other copy."""
    sizes = range(0, 129, 4)
    for left, right in sorted(((l, r) for l in sizes for r in sizes), key=lambda lr: (lr[0] + lr[1], lr[0])):
        if left + length + right < 16 or start - left < begin or start + length + right > end:
            continue
        needle = retail[start - left : start + length + right]
        found = section.find(needle)
        if found < 0 or section.find(needle, found + 1) >= 0:
            continue
        first = retail.find(needle, begin, end)
        if retail.find(needle, first + 1, end) < 0:
            return found + left
    return None


def extract_original(dtk: Path, disc: Path, original: Path) -> None:
    """Extract the disc's data partition once; extract again when the disc image changes."""
    stamp = original.with_name(original.name + ".source")
    info = disc.stat()
    key = f"{disc.resolve()} {info.st_size} {info.st_mtime_ns}\n"
    if (original / "sys/main.dol").is_file() and stamp.is_file() and stamp.read_text() == key:
        return
    if original.exists():
        shutil.rmtree(original)
    original.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([str(dtk), "disc", "extract", "-q", str(disc), str(original)], check=True)
    stamp.write_text(key)


def write_depfile(out: Path, *folders: Path) -> None:
    """List the mod's files and folders for ninja, so adding, changing or removing one rebuilds the image."""
    deps = [p for folder in folders if folder.is_dir() for p in [folder] + sorted(folder.rglob("*"))]

    def escape(path: Path) -> str:
        return path.as_posix().replace(" ", "\\ ")

    out.with_name(out.name + ".d").write_text(f"{escape(out)}: " + " ".join(escape(p) for p in deps) + "\n")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dtk", type=Path, required=True)
    parser.add_argument("--wit", type=Path, required=True)
    parser.add_argument("--disc", type=Path, required=True, help="the original disc image")
    parser.add_argument("--dol", type=Path, required=True, help="the mod's main.dol")
    parser.add_argument("--files", type=Path, required=True, help="the mod's files/ folder, may be empty")
    parser.add_argument("--work", type=Path, required=True, help="folder for the extracted and the patched tree")
    parser.add_argument("--out", type=Path, required=True, help="the disc image to write")
    parser.add_argument("--patch", type=Path, help="a translation patch folder for the retail disc: main.dol and files")
    args = parser.parse_args()
    original = args.work / "original"
    extract_original(args.dtk, args.disc, original)
    dol = args.dol
    if args.patch:
        retail = (original / "sys/main.dol").read_bytes()
        patched = (args.patch / "main.dol").read_bytes()
        dol = args.work / "main.dol"
        dol.write_bytes(port_dol(retail, patched, args.dol.read_bytes()))
    count = prepare_tree(original, args.work / "root", dol, args.files, args.patch)
    print(f"{count} file(s) from the mod")
    subprocess.run([str(args.wit), "copy", "--overwrite", str(args.work / "root"), str(args.out)], check=True)
    write_depfile(args.out, *[folder for folder in (args.files, args.patch) if folder])


if __name__ == "__main__":
    main()
