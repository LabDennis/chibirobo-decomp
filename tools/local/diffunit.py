#!/usr/bin/env python3
"""List the mismatched functions of one unit, worst first.

usage: diffunit.py <unit>           e.g. diffunit.py main/game/cfont
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from reportlib import functions, instruction_texts, section_sizes, unit_diff


def main(unit):
    diff = unit_diff(unit)
    lf, rf = functions(diff, "left"), functions(diff, "right")
    rows = []
    for name, ls in lf.items():
        rs = rf.get(name)
        if rs is None:
            rows.append((name, ls.get("size", "?"), "MISSING in build"))
            continue
        mp = rs.get("match_percent", ls.get("match_percent", 100))
        if mp is not None and mp < 100:
            rows.append((name, ls.get("size", "?"), f"match={mp:.2f}"))
    print(f"unit {unit}: {len(lf)} target functions, {len(rows)} mismatched")
    for name, size, info in rows:
        print(f"  {size!s:>7}  {info:<16}  {name}")
    lsec, rsec = section_sizes(diff, "left"), section_sizes(diff, "right")
    if lsec.keys() != rsec.keys():
        print("  sections: target", sorted(lsec), " build", sorted(rsec))


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    main(sys.argv[1])
