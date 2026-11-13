#!/usr/bin/env python3
"""Build each version with a disc in orig/ and check it hashes to retail.

python3 tools/check_versions.py [VERSION ...] [--all-source]

Reconfigures this checkout for each one and puts it back on whatever was
configured before, so don't run another build at the same time.
"""
import argparse
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import versions as ver  # noqa: E402

ROOT = ver.ROOT


def build(version, all_source=False):
    r = subprocess.run([sys.executable, "configure.py", "--version", version], cwd=ROOT, capture_output=True, text=True)
    if r.returncode:
        return False, r.stdout + r.stderr
    targets = ["all_source", "progress"] if all_source else []
    r = subprocess.run(["ninja", *targets], cwd=ROOT, capture_output=True, text=True)
    out = r.stdout + r.stderr
    return r.returncode == 0 and (ROOT / "build" / version / "ok").is_file(), out


def check(wanted=None, all_source=False):
    wanted = list(wanted or ver.buildable())
    before = ver.configured()
    results = {}
    try:
        for v in wanted:
            ok, out = build(v, all_source)
            results[v] = ok
            print(f"{v}: {'OK' if ok else 'FAILED'}", flush=True)
            if not ok:
                errors = [l for l in out.splitlines() if "FAILED" in l or "Error" in l or "undefined" in l]
                for l in list(dict.fromkeys(errors))[:10]:
                    print(f"  {l}")
    finally:
        if before:
            subprocess.run([sys.executable, "configure.py", "--version", before], cwd=ROOT, capture_output=True)
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("versions", nargs="*")
    parser.add_argument("--all-source", action="store_true", help="build all_source too")
    args = parser.parse_args()
    for v in args.versions:
        if v not in ver.versions():
            sys.exit(f"unknown version {v}")
    results = check(args.versions, args.all_source)
    return 0 if results and all(results.values()) else 1


if __name__ == "__main__":
    sys.exit(main())
