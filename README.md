# Chibi-Robo!

A work in progress decompilation of Chibi-Robo! for the GameCube and the Japanese Wii release.

[![Linked](https://cleosp.com/chibirobo-decomp/badges/linked.svg)](https://cleosp.com/chibirobo-decomp/)
[![Data](https://cleosp.com/chibirobo-decomp/badges/data.svg)](https://cleosp.com/chibirobo-decomp/)

[![Decompilation progress](https://cleosp.com/chibirobo-decomp/progress.svg)](https://cleosp.com/chibirobo-decomp/)

[Explore the interactive progress map](https://cleosp.com/chibirobo-decomp/).

Supported versions:

- `GGTE01`: GameCube, USA rev 0
- `GGTJ01_01`: GameCube, Japan rev 1
- `GGTP01`: GameCube, Europe rev 0
- `R24J01`: Wii, Japan rev 0 ([Wiiであそぶ ちびロボ！](https://www.nintendo.co.jp/wii/r24j/index.html))

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
git clone --branch wii-jp https://github.com/LabDennis/chibirobo-decomp.git
cd chibirobo-decomp
```

Put an ISO, RVZ or CISO of your game in `orig/<version>/`. For the Japanese Wii release, use `orig/R24J01/`.

Configure and build:

```sh
python3 configure.py
ninja
```

The default is the US GameCube release. To build the Wii release:

```sh
python3 configure.py --version R24J01
ninja
```

The GameCube build checks the DOL and 16 RELs. The Wii disc has no separate REL files. Its executable is `build/R24J01/main.dol` (SHA-1 `04ab6605677ce632b43d3b68d51991e6956b8023`). Unfinished units still link original code. Run `ninja progress` to see how much of the selected release builds from source.

### Wii version differences

- The Wii loads stage scripts directly from `files/bin/`. The Japanese GameCube disc stores them in `qp.bin` and loads stage code from RELs. On Wii, the DOL holds the dispatch tables for stage code.
- Each Wii stage descriptor carries a sound-bank name and the first sound ID used while loading that stage. For example, the kitchen uses `sfx_kitchen.ssm` and the archive entry `LOAD_KITCHEN1` (ID `0x245`) in `snd/cb_robo.brsar`.
- The Wii title menu supports the Wii Remote pointer. Save slots and their banner live in Wii NAND. In Dolphin, the rebuilt executable can start a new game, save to a slot and reload it after restarting.

## Progress site

The Progress workflow rebuilds the three GameCube versions on the self-hosted runner,
then publishes the interactive page and README badges to
[cleosp.com/chibirobo-decomp](https://cleosp.com/chibirobo-decomp/). It runs when
source, config, or tools change on the default branch, and can also be run from
the Actions tab. Failed builds leave the last published page in place.

The runner needs `CHIBIROBO_ORIG` set to its local `orig/` folder. Only the
progress report goes to GitHub; the discs stay on the runner. In the repo’s
Pages settings, the publishing source should be **GitHub Actions**.

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

[Credits and license notices](NOTICE)
