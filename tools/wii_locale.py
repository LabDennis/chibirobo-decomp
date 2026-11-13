"""Generate Wii interface strings. The NAND banner uses two 32-unit UTF-16 fields."""

import json
import os
from pathlib import Path

from .versions import DISC_EXT


def disc_image(folder: Path) -> Path:
    # DTK can split a disc image directly, so a successful build need not leave an extracted partition.
    images = sorted(p for p in folder.glob("*") if p.is_file() and p.suffix.lower() in DISC_EXT)
    if images:
        return images[0]
    extracted = folder / "sys/main.dol"
    if extracted.is_file() and (folder / "sys/boot.bin").is_file() and (folder / "files").is_dir():
        return extracted
    raise ValueError(f"Place an R24J01 disc image or its extracted game partition in {folder}")


def write_header(translations: Path, locale: str, output: Path) -> None:
    strings = json.loads(translations.read_text(encoding="utf-8"))[locale]
    if len(strings) != 6 or any(not isinstance(s, str) or not s or "\0" in s for s in strings):
        raise ValueError(f"{locale}: expected the six Wii title, banner and controller messages")
    lines = []
    entries = []
    for index, string in enumerate(strings):
        if index in (1, 2):
            data = string.encode("utf-16-be")
            if len(data) > 62:
                raise ValueError(f"{locale}: NAND banner line {index} exceeds 31 UTF-16 units")
            units = [f"0x{int.from_bytes(data[i:i+2], 'big'):04x}" for i in range(0, len(data), 2)]
            lines.append(f"static const u16 sBannerLine{index}[] = {{ {', '.join([*units, '0'])} }};")
            entries.append(f"reinterpret_cast<const char*>(sBannerLine{index})")
        else:
            # The retail Japanese renderer reads Shift-JIS. Keep starter prompts in its ASCII range.
            if not string.isascii() or any(ord(c) < 32 or ord(c) > 126 for c in string):
                raise ValueError(f"{locale}: Wii message {index} requires printable ASCII")
            entries.append(json.dumps(string))
    lines += ["static const char* const sLocalizedWiiInterface[] = {", *(f"    {s}," for s in entries), "};"]
    text = "\n".join(lines) + "\n"
    output.parent.mkdir(parents=True, exist_ok=True)
    if not output.exists() or output.read_text(encoding="ascii") != text:
        output.write_text(text, encoding="ascii")


def write_localization(translations: Path, locale: str, output: Path) -> None:
    write_header(translations, locale, output / "include/wii_locale.h")


def write_mod(output: Path, base: Path, features: str) -> None:
    # Dolphin mounts patched files over the original partition and retains its disc identity.
    (output / "files").mkdir(parents=True, exist_ok=True)
    xml = """<wiidisc version="1" root="/">
  <id game="R24J" developer="01" disc="0" version="0" />
  <options><section name="Chibi-Robo!"><option name="Mod" id="mod" default="1">
    <choice name="FEATURES"><patch id="mod" /></choice>
  </option></section></options>
  <patch id="mod">
    <file disc="main.dol" external="/main.dol" />
    <folder disc="/" external="/files" recursive="true" create="true" />
  </patch>
</wiidisc>
"""
    (output / "riivolution.xml").write_text(xml.replace("FEATURES", features), encoding="utf-8")
    base = base.resolve()
    try:
        relative_base = Path(os.path.relpath(base, output))
    except ValueError:  # Windows paths on different drives
        relative_base = base
    descriptor = {
        "type": "dolphin-game-mod-descriptor", "version": 1,
        "display-name": f"Chibi-Robo! (Wii, {features})",
        "base-file": relative_base.as_posix(),
        "riivolution": {"patches": [{"xml": "riivolution.xml", "root": ".",
                        "options": [{"option-id": "mod", "choice": 1}]}]},
    }
    (output / "game.json").write_text(json.dumps(descriptor, indent=2) + "\n", encoding="utf-8")
