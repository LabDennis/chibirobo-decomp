#!/usr/bin/env python3
"""Shared helpers for the local decomp tooling.

The tools work on the build tree of one configured version (default
GGTJ01_01) and wrap objdiff-cli plus the generated report.json.
"""
import json
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DEFAULT_VERSION = "GGTJ01_01"


def version():
    return os.environ.get("CHIBI_VERSION", DEFAULT_VERSION)


def build_dir(ver=None):
    return os.path.join(ROOT, "build", ver or version())


def objdiff_bin():
    exe = "objdiff-cli.exe" if os.name == "nt" else "objdiff-cli"
    return os.path.join(ROOT, "build", "tools", exe)


def load_report(ver=None):
    with open(os.path.join(build_dir(ver), "report.json"), encoding="utf-8") as f:
        return json.load(f)


def unit_diff(unit, ver=None):
    """Run objdiff-cli on one unit and return the parsed diff JSON."""
    out = os.path.join(ROOT, "build", "_diff_tmp.json")
    cmd = [objdiff_bin(), "--no-color", "diff", "-p", ROOT, "-u", unit, "-o", out]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        sys.stderr.write(proc.stderr)
        raise SystemExit(f"objdiff failed for unit {unit}")
    with open(out, encoding="utf-8") as f:
        return json.load(f)


def functions(diff, side):
    return {
        s["name"]: s
        for s in diff[side]["symbols"]
        if s.get("kind") == "SYMBOL_FUNCTION"
    }


def instruction_texts(symbol):
    return [(i.get("instruction") or {}).get("formatted", "?") for i in symbol.get("instructions", [])]


def section_sizes(diff, side):
    skip = (".symtab", ".strtab", ".shstrtab", ".comment", ".note.split")
    return {
        s["name"]: (s.get("size"), s.get("match_percent"))
        for s in diff[side]["sections"]
        if not s["name"].startswith(".rela") and s["name"] not in skip
    }
