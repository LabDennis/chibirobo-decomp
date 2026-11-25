#!/usr/bin/env python3
"""update builds every version and saves progress/data.json, render turns that into
the treemap page, svg and badges. render doesn't need the game."""

import argparse
import hashlib
import json
from html import escape
from pathlib import Path

import check_versions
import treemap
import treemap_layout
import versions

ROOT = versions.ROOT
SNAPSHOT = ROOT / "progress" / "data.json"
OUTPUT = ROOT / "progress"


def source_hash():
    paths = [ROOT / "configure.py"]
    for folder in ("src", "include", "config", "tools"):
        paths.extend(p for p in (ROOT / folder).rglob("*")
                     if p.is_file() and "__pycache__" not in p.parts
                     and p.suffix not in {".orig", ".bak", ".pyc"}
                     and (folder in {"src", "include"} or p.suffix in {".py", ".yml", ".txt", ".sha1"}))
    digest = hashlib.sha256()
    for path in sorted(paths):
        digest.update(path.relative_to(ROOT).as_posix().encode() + b"\0")
        digest.update(path.read_bytes() + b"\0")
    return digest.hexdigest()


def update(wanted):
    wanted = list(dict.fromkeys(wanted or versions.buildable()))
    if not wanted:
        raise ValueError("no discs in orig/")
    if versions.USA not in wanted:
        raise ValueError("the readme numbers come from usa, add --version GGTE01")
    checked_source_hash = source_hash()
    results = check_versions.check(wanted, all_source=True)
    if set(results) != set(wanted) or not all(results.values()):
        raise ValueError("build failed, data.json left alone")
    data = []
    for version in wanted:
        report = ROOT / "build" / version / "report.json"
        if not report.is_file():
            raise ValueError(f"missing report for {version}, data.json left alone")
        units, _ = treemap.load_units(str(report.parent), str(report))
        data.append({
            "version": version,
            "region": treemap.REGIONS.get(version[3:4], ""),
            "revision": treemap.revision_of(version),
            "source": "verified build",
            "generated": int(report.stat().st_mtime),
            "units": units,
        })
    if source_hash() != checked_source_hash:
        raise ValueError("source changed during verification, data.json left alone")
    payload = {"schema": 1, "source_hash": checked_source_hash,
               "default": versions.default_version() if versions.default_version() in wanted else wanted[0],
               "groups": treemap.GROUP_ORDER, "versions": data}
    SNAPSHOT.parent.mkdir(parents=True, exist_ok=True)
    SNAPSHOT.write_text(json.dumps(payload, ensure_ascii=False, separators=(",", ":")) + "\n", encoding="utf-8")
    print(f"updated {SNAPSHOT.relative_to(ROOT)}")


def us_units(payload):
    for version in payload["versions"]:
        if version["version"] == versions.USA:
            return version["units"]
    raise ValueError("no usa report in data.json, run update with GGTE01")


def percentages(payload):
    units = us_units(payload)
    code = sum(u["code"] for u in units)
    data = sum(u["data"] for u in units)
    return {
        "linked": 100 * sum(u["code"] for u in units if u["complete"]) / code if code else 0,
        "data": 100 * sum(u["matched_data"] for u in units) / data if data else 0,
    }


def svg(payload):
    units = us_units(payload)
    attrs = " ".join(f'{key}="{value}"' for key, value in treemap.SVG_ATTRIBUTES.items())
    border = " ".join(f'{key}="{value}"' for key, value in treemap.RECT_ATTRIBUTES.items())
    parts = [f'<svg xmlns="http://www.w3.org/2000/svg" {attrs} role="img" aria-labelledby="title desc">',
             '<title id="title">Chibi-Robo! US decompilation treemap</title>',
             '<desc id="desc">Units sized by code bytes and colored by progress.</desc>']
    for index, x, y, width, height in treemap_layout.layout([u["code"] for u in units]):
        unit = units[index]
        parts.append(f'<rect x="{x}" y="{y}" width="{width}" height="{height}" fill="{treemap_layout.unit_color(unit)}" {border}><title>{escape(unit["full"])}</title></rect>')
    parts.append('</svg>\n')
    return "\n".join(parts)


def badge(label, percent):
    value = f"{percent:.1f}%"
    left = len(label) * 7 + 12
    right = len(value) * 7 + 12
    width = left + right
    meaning = "US code linked from source" if label == "linked" else "US data matched"
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="20" role="img" aria-label="{meaning}: {value}">
<title>{meaning}: {value}</title>
<linearGradient id="shade" x2="0" y2="100%"><stop offset="0" stop-color="#fff" stop-opacity=".1"/><stop offset="1" stop-opacity=".1"/></linearGradient>
<clipPath id="clip"><rect width="{width}" height="20" rx="3"/></clipPath>
<g clip-path="url(#clip)"><path fill="#555" d="M0 0h{left}v20H0z"/><path fill="#1f5f4b" d="M{left} 0h{right}v20H{left}z"/><path fill="url(#shade)" d="M0 0h{width}v20H0z"/></g>
<g fill="#fff" text-anchor="middle" font-family="Verdana, sans-serif" font-size="11">
<text x="{left / 2}" y="15" fill="#010101" fill-opacity=".3">{escape(label)}</text><text x="{left / 2}" y="14">{escape(label)}</text>
<text x="{left + right / 2}" y="15" fill="#010101" fill-opacity=".3">{value}</text><text x="{left + right / 2}" y="14">{value}</text>
</g></svg>
'''


def render(output=OUTPUT):
    payload = json.loads(SNAPSHOT.read_text(encoding="utf-8"))
    if payload.get("schema") != 1:
        raise ValueError("data.json is from a different schema")
    measures = percentages(payload)
    stale = payload["source_hash"] != source_hash()
    payload["live"] = False
    if stale:
        for version in payload["versions"]:
            version["source"] = "earlier build; source has changed"
    output.mkdir(parents=True, exist_ok=True)
    page = treemap.render_page(payload)
    (output / "index.html").write_text(page, encoding="utf-8")
    # Keep old README links and bookmarks working, including their filters.
    (output / "progress.html").write_text(page, encoding="utf-8")
    (output / "progress.svg").write_text(svg(payload), encoding="utf-8")
    (output / "badges").mkdir(exist_ok=True)
    for label, percent in measures.items():
        (output / "badges" / f"{label}.svg").write_text(badge(label, percent), encoding="utf-8")
    print(f"rendered treemap and README badges in {output}")
    if stale:
        print("source changed since data.json was made, run update")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("update", "render"))
    parser.add_argument("--version", action="append", choices=versions.versions(), help="can be given more than once")
    parser.add_argument("--output", type=Path, default=OUTPUT, help="where render writes")
    args = parser.parse_args()
    try:
        if args.command == "update":
            update(args.version)
        elif args.version:
            parser.error("--version only applies to update")
        render(args.output)
    except (ValueError, OSError) as error:
        parser.exit(1, f"{error}\n")


if __name__ == "__main__":
    main()
