# Version and configuration information from configure.py and build.ninja.
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
USA = "GGTE01"
DISC_EXT = (".ciso", ".iso", ".gcm", ".rvz", ".wbfs", ".wia", ".nfs", ".tgc")
CONFIGURE_ARGS_PREFIX = "# configure argv: "


def _configure_text():
    return (ROOT / "configure.py").read_text(encoding="utf-8")


def versions():
    block = re.search(r"^VERSIONS = \[(.*?)^\]", _configure_text(), re.M | re.S).group(1)
    return re.findall(r'"(\w+)"', block)


def default_version():
    m = re.search(r"^DEFAULT_VERSION = (\d+)", _configure_text(), re.M)
    return versions()[int(m.group(1)) if m else 0]


def configured():
    # None before the first configure
    ninja = ROOT / "build.ninja"
    if not ninja.exists():
        return None
    m = re.search(r"-DVERSION_(\w+)", ninja.read_text(encoding="utf-8"))
    return m.group(1) if m else None


def configured_arguments():
    try:
        text = (ROOT / "build.ninja").read_text(encoding="utf-8")
    except FileNotFoundError:
        return None
    for line in text.splitlines():
        if line.startswith(CONFIGURE_ARGS_PREFIX):
            return json.loads(line.removeprefix(CONFIGURE_ARGS_PREFIX))

    # Older manifests only recorded the space-separated Ninja variable.
    text = re.sub(r" \$\n[ \t]+", " ", text)
    match = re.search(r"^configure_args =([^\n]*)", text, re.M)
    return match.group(1).split() if match else None


def current():
    return configured() or default_version()


def has_disc(version):
    base = ROOT / "orig" / version
    return base.is_dir() and any(p.suffix.lower() in DISC_EXT for p in base.iterdir())


def buildable():
    return [v for v in versions() if has_disc(v)]
