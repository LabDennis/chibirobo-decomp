#!/usr/bin/env python3
"""Build and publish release reports, or merge a separately verified local report
into an existing snapshot. Rendering a snapshot does not require a disc."""

import argparse
import hashlib
import json
import tempfile
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


def read_release(version, source, digest=None):
    folder = ROOT / "build" / version
    report = folder / "report.json"
    if not report.is_file() or not (folder / "ok").is_file():
        raise ValueError(f"{version} needs a verified build and report.json")
    units, _ = treemap.load_units(str(folder), str(report))
    if not units:
        raise ValueError(f"{version} has no units in report.json")
    return {
        "version": version,
        "region": treemap.REGIONS.get(version[3:4], ""),
        "revision": treemap.revision_of(version),
        "source": source,
        "source_hash": digest,
        "generated": int(report.stat().st_mtime),
        "units": units,
    }


def write_snapshot(path, payload):
    path.parent.mkdir(parents=True, exist_ok=True)
    text = json.dumps(payload, ensure_ascii=False, separators=(",", ":")) + "\n"
    # Readers can keep using the previous snapshot until the complete file is ready.
    temporary = None
    try:
        with tempfile.NamedTemporaryFile("w", dir=path.parent, encoding="utf-8", delete=False) as handle:
            temporary = Path(handle.name)
            handle.write(text)
        temporary.replace(path)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def update(wanted, snapshot):
    wanted = list(dict.fromkeys(wanted or versions.buildable()))
    if not wanted:
        raise ValueError("no discs in orig/")
    if versions.USA not in wanted:
        raise ValueError("the README badges need the USA build, add --version GGTE01")
    results = check_versions.check(wanted, all_source=True)
    if set(results) != set(wanted) or not all(results.values()):
        raise ValueError("build failed, snapshot left alone")
    digest = source_hash()
    data = [read_release(version, "verified build", digest) for version in wanted]
    default = versions.default_version()
    write_snapshot(snapshot, {"schema": 1, "source_hash": digest,
                              "default": default if default in wanted else wanted[0],
                              "groups": treemap.GROUP_ORDER, "versions": data})


def merge(wanted, snapshot):
    if not wanted:
        raise ValueError("merge needs at least one --version")
    payload = json.loads(snapshot.read_text(encoding="utf-8"))
    if payload.get("schema") != 1:
        raise ValueError("unsupported progress snapshot schema")
    releases = {release["version"]: release for release in payload["versions"]}
    if len(releases) != len(payload["versions"]):
        raise ValueError("duplicate release in progress snapshot")
    # Older snapshots recorded one hash for the entire build. Keep its original
    # scope when importing a report produced separately.
    previous_hash = payload.pop("source_hash", None)
    for release in releases.values():
        release.setdefault("source_hash", previous_hash)
    for version in dict.fromkeys(wanted):
        release = read_release(version, "locally verified build report")
        manifest = ROOT / "config" / version / "build.sha1"
        entries = [line.split(maxsplit=1) for line in manifest.read_text().splitlines() if line.strip()]
        if not entries:
            raise ValueError(f"empty retail checksum manifest for {version}")
        for checksum, filename in entries:
            digest = hashlib.sha1()
            with (ROOT / filename).open("rb") as binary:
                for chunk in iter(lambda: binary.read(1024 * 1024), b""):
                    digest.update(chunk)
            if digest.hexdigest().lower() != checksum.lower():
                raise ValueError(f"{filename} differs from its retail checksum")
        # report.json and the ok marker carry no source-tree fingerprint.
        # Keep the imported report's timestamp and leave its source hash unknown.
        releases[version] = release
    payload["versions"] = list(releases.values())
    write_snapshot(snapshot, payload)


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


def render(output=OUTPUT, snapshot=SNAPSHOT, source_url=None, site_url="./"):
    raw = snapshot.read_bytes()
    payload = json.loads(raw)
    if payload.get("schema") != 1:
        raise ValueError("data.json is from a different schema")
    measures = percentages(payload)
    digest = source_hash()
    payload["live"] = False
    for release in payload["versions"]:
        release_hash = release.get("source_hash", payload.get("source_hash"))
        if release_hash and release_hash != digest:
            release["source"] = "earlier verified build (source has changed)"
    output.mkdir(parents=True, exist_ok=True)
    if (output / "data.json").resolve() != snapshot.resolve():
        (output / "data.json").write_bytes(raw)
    page = treemap.render_page(payload, source_url, site_url)
    (output / "index.html").write_text(page, encoding="utf-8")
    # Keep old README links and bookmarks working, including their filters.
    (output / "progress.html").write_text(page, encoding="utf-8")
    (output / "progress.svg").write_text(svg(payload), encoding="utf-8")
    (output / "badges").mkdir(exist_ok=True)
    for label, percent in measures.items():
        (output / "badges" / f"{label}.svg").write_text(badge(label, percent), encoding="utf-8")
    print(f"rendered treemap and README badges in {output}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("update", "merge", "render"))
    parser.add_argument("--version", action="append", choices=versions.versions(), help="can be given more than once")
    parser.add_argument("--snapshot", type=Path, default=SNAPSHOT, help="snapshot to read or update")
    parser.add_argument("--source-url", help="repository or branch linked from the treemap")
    parser.add_argument("--site-url", default="./", help="published Pages URL for canonical links and sharing")
    parser.add_argument("--output", type=Path, default=OUTPUT, help="where render writes")
    args = parser.parse_args()
    try:
        if args.command == "update":
            update(args.version, args.snapshot)
        elif args.command == "merge":
            merge(args.version, args.snapshot)
        elif args.version:
            parser.error("--version only applies to update or merge")
        render(args.output, args.snapshot, args.source_url, args.site_url)
    except (ValueError, OSError) as error:
        parser.exit(1, f"{error}\n")


if __name__ == "__main__":
    main()
