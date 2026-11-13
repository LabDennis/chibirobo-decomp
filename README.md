# Chibi-Robo!

A work in progress decompilation of Chibi-Robo! for the GameCube and Wii.

[![Linked](https://cleosp.com/chibirobo-decomp/badges/linked.svg)](https://cleosp.com/chibirobo-decomp/)
[![Data](https://cleosp.com/chibirobo-decomp/badges/data.svg)](https://cleosp.com/chibirobo-decomp/)
[![Wii JP linked](https://cleosp.com/chibirobo-decomp/badges/wii-linked.svg)](https://cleosp.com/chibirobo-decomp/#v=R24J01)

[![Decompilation progress](https://cleosp.com/chibirobo-decomp/progress.svg)](https://cleosp.com/chibirobo-decomp/)

[Explore the interactive progress map](https://cleosp.com/chibirobo-decomp/).

Supported versions:

- `GGTE01`: USA rev 0
- `GGTJ01_01`: Japan rev 1
- `GGTP01`: Europe rev 0
- `R24J01`: Japan rev 0 (Wii)

This repository contains no game assets. You need your own copy of the game to build it.

## Dependencies

### Windows

Install [Python](https://www.python.org/downloads/) and add it to PATH, then install ninja with `py -m pip install ninja`. Use `py` instead of `python3` below.

### macOS

```sh
brew install python ninja
```

### Linux

Install Python 3.10 or later and ninja with your package manager.

The build downloads its compiler and other tools on first run.

## Building

Clone the repository:

```sh
git clone https://github.com/CleoSp/chibirobo-decomp.git
cd chibirobo-decomp
```

Put an ISO, RVZ or CISO of your game in `orig/<version>/`, for example `orig/R24J01/` for Wii.

Configure and build:

```sh
python3 configure.py
ninja
```

To pick a version, use its ID when configuring, for example `python3 configure.py --version R24J01`. The default is `GGTE01`.

The build checks the DOL and, on GameCube, the 16 RELs against the originals. Unfinished units still link original code.

## Progress site

The Progress workflow rebuilds the three GameCube versions on the self-hosted runner,
plus [Wii Japan](https://cleosp.com/chibirobo-decomp/#v=R24J01) when the runner has that disc,
then publishes the interactive page and README badges to
[cleosp.com/chibirobo-decomp](https://cleosp.com/chibirobo-decomp/). It runs when
source, config, or tools change on the default branch, and can also be run from
the Actions tab. Failed builds leave the last published page in place.

The runner needs `CHIBIROBO_ORIG` set to its local `orig/` folder, with a
subfolder per version as in this repository. Without `R24J01/` the Wii badge
reads "not built". Only the progress report goes to GitHub; the discs stay on
the runner. In the repo’s Pages settings, the publishing source should be
**GitHub Actions**.

## Diffing

Once the first build succeeds, open [objdiff](https://github.com/encounter/objdiff/releases), set its project directory to your clone and select a unit. objdiff reads the generated `objdiff.json` and rebuilds automatically when source, header or config files change.

Before opening a pull request, compile all source:

```sh
ninja all_source progress
```

Include your source and config changes in the pull request. Keep game files and build output out of it.

## Modding

Configure with `--non-matching` to build modified or non-matching code:

```sh
python3 configure.py --non-matching
ninja
```

The output DOL is written to `build/GGTE01/main.dol`, or to the folder for the version you built.

### Wii mods

Two options build a mod of your Japanese Wii disc, alone or together:

- `--gcncon`: GameCube controller support. On the first screen, the first A
  press picks the controls: A on a GameCube controller in port 1 plays with the
  GameCube controller alone, and the game stops asking for the Wii Remote; A on
  the Wii Remote plays as the Wii release does, pointer, motion and speaker
  included. The game already maps the GameCube buttons; the release only left
  them switched off.
- `--langpack <locale>`: the game's text in another language (see below).

```sh
python3 configure.py --version R24J01 --gcncon --langpack en-US
ninja
```

Open `build/wii-en-US-gcncon/game.json` in Dolphin (the folder is named after the
options: `wii-gcncon`, `wii-en-US`, …). Dolphin applies the mod to your original
disc image; on a Wii, Riivolution does the same with the folder's
`riivolution.xml` and `main.dol`.

For `--langpack`, choose `en-US`, `en-GB`, `de-DE`, `fr-FR`, `es-ES` or `it-IT`.
The starter translations cover the Wii title, save banner and three Wii Remote
messages. Other text and assets fall back to the Japanese release.
For new translations, edit `mods/wii-localization/interface.json` or place
Wii-compatible replacement assets under the build's `files/` folder, using
their original disc paths (for example `files/bin/stage14.bin`).
These mods share the Japanese title's save files. Use a separate Dolphin user
directory to keep a development save. Retail progress is measured against `R24J01`.

To import actor, costume and stage names, extract the desired language's
`globals.bin` from your GameCube disc's `qp.bin`. Pass that file and the
original Wii file to the importer. For an English build:

```sh
python3 tools/wii_stage_names.py path/to/gc/globals.bin orig/R24J01/files/bin/globals.bin build/wii-en-US/files/bin/globals.bin
```

The importer lists skipped IDs and keeps their Japanese names. It accepts
printable ASCII by default. Add `--fold-cp1252` to convert supported Western
accents to ASCII. Object collisions, stage descriptions and event scripts
come from the Wii file.

[Credits and license notices](NOTICE)
