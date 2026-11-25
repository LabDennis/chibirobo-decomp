#!/usr/bin/env python3
"""Build every version, check formatting and look for disc files.

python3 tools/check.py                 everything
python3 tools/check.py <files>         just format these
python3 tools/check.py --source-only   skip the build, what CI runs
python3 tools/check.py --fix           clang-format whatever fails
"""

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_versions  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
CLANG_FORMAT_VERSION = "18.1.8"
# imported libraries keep their upstream formatting
SOURCE_DIRS = ("src/game", "src/rel", "include/game")
SOURCE_EXTS = {".c", ".cp", ".cpp", ".h", ".hpp", ".inc"}

SKIP_DIRS = {".git", "build", "orig", "__pycache__", ".mypy_cache", ".cache", "node_modules"}
DISC_EXTS = {
    ".iso", ".gcm", ".ciso", ".rvz", ".wbfs", ".nkit", ".gcz", ".wia", ".nfs", ".tgc", ".wad",
    ".dol", ".rel", ".elf", ".bin", ".arc", ".szs", ".thp",
    ".bti", ".tpl", ".bmd", ".bdl", ".dat", ".sdat", ".brsar", ".adp", ".dsp", ".ast",
}
BIG_FILE = 4 * 1024 * 1024


def find_clang_format():
    # pip --user puts it somewhere that usually isn't on PATH
    candidates = [os.environ.get("CLANG_FORMAT"), shutil.which("clang-format")]
    for minor in range(20, 7, -1):
        candidates.append(str(Path.home() / "Library" / "Python" / f"3.{minor}" / "bin" / "clang-format"))
    candidates.append(str(Path.home() / ".local" / "bin" / "clang-format"))
    for c in candidates:
        if c and Path(c).is_file():
            return c
    return None


def source_files():
    files = []
    for d in SOURCE_DIRS:
        files += [p for p in sorted((ROOT / d).rglob("*")) if p.suffix.lower() in SOURCE_EXTS]
    files += [p for p in sorted((ROOT / "include").glob("*")) if p.is_file() and p.suffix.lower() in SOURCE_EXTS]
    return files


def check_build():
    print("== build")
    try:
        results = check_versions.check(all_source=True)
    except (OSError, ValueError) as error:
        print(error)
        return False
    return bool(results) and all(results.values())


def check_format(files, fix):
    print("== format")
    exe = find_clang_format()
    if exe is None:
        print(f"no clang-format, pip install clang-format=={CLANG_FORMAT_VERSION} or set CLANG_FORMAT")
        return False
    version = subprocess.run([exe, "--version"], capture_output=True, text=True).stdout
    if CLANG_FORMAT_VERSION not in version.split():
        print(f"need clang-format {CLANG_FORMAT_VERSION}, got {version.strip()}")
        return False

    bad = []
    # chunked so the command line doesn't get too long on windows
    for i in range(0, len(files), 200):
        chunk = files[i : i + 200]
        out = subprocess.run([exe, "--dry-run", "--Werror", *map(str, chunk)], cwd=ROOT, capture_output=True, text=True)
        bad += [f for f in chunk if f"{f}:" in out.stderr]
    if bad and fix:
        subprocess.run([exe, "-i", *map(str, bad)], cwd=ROOT, check=True)
        print(f"formatted {len(bad)} file(s)")
        return check_format(files, fix=False)
    for f in bad:
        print(f"needs formatting: {f.relative_to(ROOT)}")
    print(f"{len(files)} file(s), {len(bad)} need formatting")
    return not bad


def check_disc_files():
    print("== disc files")
    bad = []
    for dirpath, dirnames, filenames in os.walk(ROOT):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
        for name in filenames:
            p = Path(dirpath) / name
            if p.suffix.lower() in DISC_EXTS:
                bad.append(f"{p.relative_to(ROOT)}: disc file type")
            elif p.stat().st_size > BIG_FILE:
                bad.append(f"{p.relative_to(ROOT)}: over 4 MB, from the disc?")
    for b in bad:
        print(b)
    return not bad


def main():
    sys.stdout.reconfigure(line_buffering=True)  # so the == headers stay in order with ninja's output
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("files", nargs="*", type=Path)
    parser.add_argument("--source-only", action="store_true", help="skip the build")
    parser.add_argument("--fix", action="store_true", help="clang-format files that need it")
    args = parser.parse_args()

    files = [f.resolve() for f in args.files if f.suffix.lower() in SOURCE_EXTS] if args.files else source_files()
    results = {}
    if not args.source_only and not args.files:
        results["build"] = check_build()
    results["format"] = check_format(files, args.fix)
    results["disc files"] = check_disc_files()

    print("\n" + "  ".join(f"{name}: {'ok' if ok else 'FAILED'}" for name, ok in results.items()))
    sys.exit(0 if all(results.values()) else 1)


if __name__ == "__main__":
    main()
