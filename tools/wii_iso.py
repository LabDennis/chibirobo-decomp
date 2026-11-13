"""Build a Wii disc image from the original disc and a mod's main.dol and files.

dtk extracts the original once; every build then links a fresh tree from it, puts the mod's files in, and has
Wiimms ISO Tools (wit) write and encrypt the image. Ticket and TMD come out fake-signed, which Dolphin accepts.
"""

import argparse
import os
import shutil
import subprocess
from pathlib import Path


def prepare_tree(original: Path, root: Path, dol: Path, files: Path) -> int:
    """Link `root` from `original`, then replace sys/main.dol and overlay `files`; the original stays untouched.
    Returns the number of files taken from the mod."""
    if root.exists():
        shutil.rmtree(root)
    shutil.copytree(original, root, copy_function=os.link)
    replaced = [(dol, root / "sys/main.dol")]
    if files.is_dir():
        replaced += [(p, root / "files" / p.relative_to(files)) for p in files.rglob("*") if p.is_file()]
    for source, target in replaced:
        # unlink first: target is a hard link into the original tree
        if target.exists():
            target.unlink()
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
    return len(replaced)


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


def write_depfile(out: Path, files: Path) -> None:
    """List the mod's files and folders for ninja, so adding, changing or removing one rebuilds the image."""
    deps = [files] + sorted(files.rglob("*")) if files.is_dir() else []

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
    args = parser.parse_args()
    original = args.work / "original"
    extract_original(args.dtk, args.disc, original)
    count = prepare_tree(original, args.work / "root", args.dol, args.files)
    print(f"{count} file(s) from the mod")
    subprocess.run([str(args.wit), "copy", "--overwrite", str(args.work / "root"), str(args.out)], check=True)
    write_depfile(args.out, args.files)


if __name__ == "__main__":
    main()
