#!/usr/bin/env python3
"""Side-by-side instruction diff for the mismatched functions of a unit.

usage: diffshow.py <unit> [function]
"""
import itertools
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from reportlib import functions, instruction_texts, unit_diff


def main(unit, fname=None):
    diff = unit_diff(unit)
    lf, rf = functions(diff, "left"), functions(diff, "right")
    names = [fname] if fname else [
        n for n, s in lf.items()
        if n in rf and (rf[n].get("match_percent", 100) or 100) < 100
    ]
    for name in names:
        ls, rs = lf[name], rf[name]
        li, ri = instruction_texts(ls), instruction_texts(rs)
        print(f"=== {name}  target_size={ls.get('size')} match={rs.get('match_percent')} ===")
        width = max((len(t) for t in li), default=0)
        for a, b in itertools.zip_longest(li, ri, fillvalue=""):
            mark = "  " if a == b else "**"
            print(f"{mark} {a:<{width}} | {b}")
        print()


if __name__ == "__main__":
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    main(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else None)
