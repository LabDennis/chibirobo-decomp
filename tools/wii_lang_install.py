"""Synchronize the generated language subtree into one mod's files directory."""
import os
import shutil
import sys
from pathlib import Path
from tempfile import TemporaryDirectory


def install(files, target):
    source = files / "lang"
    for code in ("en", "de", "fr", "es", "it"):
        if not (source / code).is_dir():
            raise ValueError(f"Missing generated language directory: {source / code}")
    # Only the generated lang subtree belongs to this rule. Keep sibling mod files.
    target.parent.mkdir(parents=True, exist_ok=True)
    with TemporaryDirectory(dir=target.parent) as tmp:
        staged = Path(tmp) / "lang"
        shutil.copytree(source, staged)
        old = Path(tmp) / "old"
        if target.exists():
            target.rename(old)
        try:
            staged.rename(target)
        except OSError:
            if old.exists():
                old.rename(target)
            raise
    os.utime(target)


if __name__ == "__main__":
    install(Path(sys.argv[1]), Path(sys.argv[2]))
