#!/usr/bin/env python3
"""Try flipping one unit from NonMatching to Matching and verify the build.

Edits configure.py in place, reconfigures, builds, and reports whether the
linked dol still matches byte for byte. On failure the edit is reverted, so
the tree always ends in the last good state. splits.txt changes needed for
data sections must be made before running this.

usage: flipcheck.py <source.cpp>     e.g. flipcheck.py game/cfont.cpp
"""
import json
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from reportlib import ROOT, build_dir, load_report, version


def run(cmd, **kw):
    return subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, **kw)


def measures(ver):
    try:
        m = load_report(ver)["measures"]
        return {k: m.get(k) for k in ("fuzzy_match_percent", "matched_code", "matched_data",
                                      "matched_functions", "complete_units")}
    except (OSError, KeyError, json.JSONDecodeError):
        return None


def main(source, ver):
    configure = os.path.join(ROOT, "configure.py")
    with open(configure, encoding="utf-8") as f:
        text = f.read()
    pattern = re.compile(r"Object\(NonMatching, (\"" + re.escape(source) + r"\")")
    if not pattern.search(text):
        raise SystemExit(f'no Object(NonMatching, "{source}") line in configure.py')
    before = measures(ver)
    with open(configure, "w", encoding="utf-8") as f:
        f.write(pattern.sub(r"Object(Matching, \1", text))
    ok = False
    try:
        proc = run([sys.executable, "configure.py", "--version", ver])
        if proc.returncode != 0:
            print(proc.stdout[-2000:], proc.stderr[-2000:])
            raise SystemExit("configure failed, reverted")
        proc = run([sys.executable, "-m", "ninja"], timeout=900)
        tail = (proc.stdout or "")[-1500:]
        print(tail)
        ok = proc.returncode == 0 and "OK" in tail.upper()
        after = measures(ver)
        print("build:", "MATCHES" if ok else "FAILED")
        print("measures before:", before)
        print("measures after: ", after)
        if ok:
            print(f"KEEP: {source} flips to Matching cleanly")
    finally:
        if not ok:
            with open(configure, encoding="utf-8") as f:
                current = f.read()
            with open(configure, "w", encoding="utf-8") as f:
                f.write(current.replace(f'Object(Matching, "{source}"', f'Object(NonMatching, "{source}"'))
            run([sys.executable, "configure.py", "--version", ver])
            run([sys.executable, "-m", "ninja"], timeout=900)
            print("reverted to NonMatching and rebuilt")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    main(sys.argv[1], version())
