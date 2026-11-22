#!/usr/bin/env python3
"""Prioritized work list from the latest report.json.

Lists every incomplete unit with its unmatched code/data bytes, smallest
first, and flags units whose code already matches (pure pool/data work).

usage: worklist.py [--all]     # --all also lists fully matched units
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from reportlib import load_report


def as_int(value):
    return int(value) if value is not None else 0


def main(show_all):
    report = load_report()
    rows = []
    for unit in report["units"]:
        meta = unit.get("metadata", {})
        if meta.get("auto_generated"):
            continue
        m = unit["measures"]
        total_code = as_int(m.get("total_code"))
        total_data = as_int(m.get("total_data"))
        fuzzy = m.get("fuzzy_match_percent") or 100.0
        matched_data = as_int(m.get("matched_data"))
        code_left = round(total_code * (100.0 - fuzzy) / 100.0)
        data_left = total_data - matched_data
        complete = meta.get("complete", False)
        if complete and not show_all:
            continue
        funcs = unit.get("functions") or []
        bad_funcs = [f for f in funcs if (f.get("fuzzy_match_percent") or 100.0) < 100.0]
        if fuzzy < 100.0 and code_left == 0 and total_code > 0:
            code_left = 1  # subpercent remainder, keep it visible
        if matched_data < total_data and data_left == 0 and total_data > 0:
            data_left = 1
        matched_code = as_int(m.get("matched_code"))
        flip_ready = (not complete and len(bad_funcs) == 0 and fuzzy == 100.0
                      and m.get("matched_data_percent") == 100.0)
        rows.append({
            "name": unit["name"],
            "left": code_left + data_left,
            "code_left": code_left,
            "data_left": data_left,
            "bad_funcs": len(bad_funcs),
            "complete": complete,
            "flip_ready": flip_ready,
            "source": meta.get("source_path", "?"),
        })
    rows.sort(key=lambda r: r["left"])
    for r in rows:
        flag = "DONE" if r["complete"] else ("FLIP" if r["flip_ready"] else ("DATA" if r["bad_funcs"] == 0 else "    "))
        print(f"{flag} {r['left']:>7} left  (code {r['code_left']:>5}, data {r['data_left']:>6}, "
              f"fns {r['bad_funcs']:>2})  {r['name']}")


if __name__ == "__main__":
    main("--all" in sys.argv)
