#!/usr/bin/env python3
"""Build each version with a disc in orig/ and check it hashes to retail.

python3 tools/check_versions.py [VERSION ...] [--all-source]

Runs matching builds, then restores the previous configure arguments and
objdiff symbol mappings. Run checks and builds sequentially in a given checkout.
"""
import argparse
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import versions as ver  # noqa: E402

ROOT = ver.ROOT


def configure(arguments):
    return subprocess.run(
        [sys.executable, "configure.py", *arguments],
        cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
    )


def build(version, all_source=False):
    configured = configure(["--version", version])
    if configured.returncode:
        return False, configured.stdout

    targets = [f"build/{version}/ok"]
    if all_source:
        targets.extend(["all_source", "progress"])
    built = subprocess.run(
        ["ninja", *targets],
        cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
    )
    return built.returncode == 0, built.stdout


def check(wanted=None, all_source=False):
    wanted = list(wanted or ver.buildable())
    if not wanted:
        return {}
    before = ver.configured_arguments()
    objdiff = ROOT / "objdiff.json"
    try:
        local_objdiff = objdiff.read_bytes()
    except FileNotFoundError:
        local_objdiff = None

    results = {}
    try:
        for v in wanted:
            ok, out = build(v, all_source)
            results[v] = ok
            print(f"{v}: {'OK' if ok else 'FAILED'}", flush=True)
            if not ok:
                lines = out.splitlines()
                errors = [
                    line for line in lines
                    if any(word in line.lower() for word in ("failed", "error", "undefined"))
                ]
                for line in list(dict.fromkeys(errors))[:10] or lines[-10:]:
                    print(f"  {line}")
    finally:
        if local_objdiff is not None:
            objdiff.write_bytes(local_objdiff)
        if before is not None:
            restored = configure(before)
            if restored.returncode:
                raise ValueError(f"could not restore the previous build configuration\n{restored.stdout}")
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("versions", nargs="*")
    parser.add_argument("--all-source", action="store_true", help="build all_source too")
    args = parser.parse_args()
    for v in args.versions:
        if v not in ver.versions():
            sys.exit(f"unknown version {v}")
    try:
        results = check(args.versions, args.all_source)
    except (OSError, ValueError) as error:
        parser.exit(1, f"{error}\n")
    return 0 if results and all(results.values()) else 1


if __name__ == "__main__":
    sys.exit(main())
