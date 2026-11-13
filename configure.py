#!/usr/bin/env python3

###
# Generates build files for the project.
# This file also includes the project configuration,
# such as compiler flags and the object matching status.
#
# Usage:
#   python3 configure.py
#   ninja
#
# Append --help to see available options.
###

import argparse
import sys
from pathlib import Path
from typing import Any, Dict, List

from tools.project import (
    Object,
    ProgressCategory,
    ProjectConfig,
    calculate_progress,
    generate_build,
    is_windows,
)

# Game versions
DEFAULT_VERSION = 0
VERSIONS = [
    "GGTE01",  # USA rev 0
    "GGTJ01_01",  # Japan rev 1
    "GGTP01",  # Europe
]

parser = argparse.ArgumentParser()
parser.add_argument(
    "mode",
    choices=["configure", "progress"],
    default="configure",
    help="script mode (default: configure)",
    nargs="?",
)
parser.add_argument(
    "-v",
    "--version",
    choices=VERSIONS,
    type=str.upper,
    default=VERSIONS[DEFAULT_VERSION],
    help="version to build",
)
parser.add_argument(
    "--build-dir",
    metavar="DIR",
    type=Path,
    default=Path("build"),
    help="base build directory (default: build)",
)
parser.add_argument(
    "--binutils",
    metavar="BINARY",
    type=Path,
    help="path to binutils (optional)",
)
parser.add_argument(
    "--compilers",
    metavar="DIR",
    type=Path,
    help="path to compilers (optional)",
)
parser.add_argument(
    "--map",
    action="store_true",
    help="generate map file(s)",
)
parser.add_argument(
    "--debug",
    action="store_true",
    help="build with debug info (non-matching)",
)
if not is_windows():
    parser.add_argument(
        "--wrapper",
        metavar="BINARY",
        type=Path,
        help="path to wibo or wine (optional)",
    )
parser.add_argument(
    "--dtk",
    metavar="BINARY | DIR",
    type=Path,
    help="path to decomp-toolkit binary or source (optional)",
)
parser.add_argument(
    "--objdiff",
    metavar="BINARY | DIR",
    type=Path,
    help="path to objdiff-cli binary or source (optional)",
)
parser.add_argument(
    "--sjiswrap",
    metavar="EXE",
    type=Path,
    help="path to sjiswrap.exe (optional)",
)
parser.add_argument(
    "--ninja",
    metavar="BINARY",
    type=Path,
    help="path to ninja binary (optional)",
)
parser.add_argument(
    "--verbose",
    action="store_true",
    help="print verbose output",
)
parser.add_argument(
    "--non-matching",
    dest="non_matching",
    action="store_true",
    help="builds equivalent (but non-matching) or modded objects",
)
parser.add_argument(
    "--warn",
    dest="warn",
    type=str,
    choices=["all", "off", "error"],
    help="how to handle warnings",
)
parser.add_argument(
    "--no-progress",
    dest="progress",
    action="store_false",
    help="disable progress calculation",
)
args = parser.parse_args()

config = ProjectConfig()
config.version = str(args.version)
version_num = VERSIONS.index(config.version)

# Apply arguments
config.build_dir = args.build_dir
config.dtk_path = args.dtk
config.objdiff_path = args.objdiff
config.binutils_path = args.binutils
config.compilers_path = args.compilers
# Required by fill_padding.py.
config.generate_map = True
# PAL pads with zeros; US and JP use ASCII '0'.
config.fill_padding = config.version != "GGTP01"
config.non_matching = args.non_matching
config.sjiswrap_path = args.sjiswrap
config.ninja_path = args.ninja
config.progress = args.progress
if not is_windows():
    config.wrapper = args.wrapper
# Don't build asm unless we're --non-matching
if not config.non_matching:
    config.asm_dir = None

# Tool versions
config.binutils_tag = "2.42-2"
config.compilers_tag = "20251118"
config.dtk_tag = "v1.8.4"
config.objdiff_tag = "v3.8.2"
config.sjiswrap_tag = "v1.2.2"
# wibo 1.2.0 cannot load sjiswrap.exe on macOS.
config.wibo_tag = "1.0.3"

# Project
config.config_path = Path("config") / config.version / "config.yml"
config.check_sha_path = Path("config") / config.version / "build.sha1"
config.asflags = [
    "-mgekko",
    "--strip-local-absolute",
    "-I include",
    f"-I build/{config.version}/include",
    f"--defsym BUILD_VERSION={version_num}",
]
config.ldflags = [
    "-fp hardware",
    "-nodefaults",
]
if args.debug:
    config.ldflags.append("-g")  # Or -gdwarf-2 for Wii linkers
if args.map:
    config.ldflags.append("-mapunused")
    # config.ldflags.append("-listclosure") # For Wii linkers

# Use for any additional files that should cause a re-configure when modified
config.reconfig_deps = []

# Optional numeric ID for decomp.me preset
# Can be overridden in libraries or objects
config.scratch_preset_id = None

# Base flags, common to most GC/Wii games.
# Generally leave untouched, with overrides added below.
cflags_base = [
    "-nodefaults",
    "-proc gekko",
    "-align powerpc",
    "-enum int",
    "-fp hardware",
    "-Cpp_exceptions off",
    "-O4,p",
    "-inline auto",
    '-pragma "cats off"',
    '-pragma "warn_notinlined off"',
    "-maxerrors 1",
    "-nosyspath",
    "-RTTI off",
    "-fp_contract on",
    "-str reuse",
    "-multibyte",  # For Wii compilers, replace with `-enc SJIS`
    "-i include",
    "-i include/stl",
    f"-i build/{config.version}/include",
    f"-DBUILD_VERSION={version_num}",
    f"-DVERSION_{config.version}",
    f"-DDTK_CONFIG_NONMATCHING={config.non_matching:d}",
]

# Debug flags
if args.debug:
    # Or -sym dwarf-2 for Wii compilers
    cflags_base.extend(["-sym on", "-DDEBUG=1"])
else:
    cflags_base.append("-DNDEBUG=1")

# Warning flags
if args.warn == "all":
    cflags_base.append("-W all")
elif args.warn == "off":
    cflags_base.append("-W off")
elif args.warn == "error":
    cflags_base.append("-W error")

cflags_game = [
    *cflags_base,
    "-RTTI on",
    "-use_lmw_stmw on",
    "-str reuse,readonly",
]

cflags_game_eh = [
    *cflags_game,
    "-Cpp_exceptions on",
]

# REL flags
cflags_rel = [
    *cflags_game,
    "-sdata 0",
    "-sdata2 0",
    "-pool off",
]

cflags_hsd = [
    *cflags_base,
]


def with_hsd_headers(flags: List[str]) -> List[str]:
    # Search HSD first to avoid Dolphin/dolphin collisions on case-insensitive disks.
    i = flags.index("-i include")
    return [*flags[:i], "-i include/hsd", *flags[i:]]


cflags_hsd = with_hsd_headers(cflags_hsd)
cflags_hsd_game = with_hsd_headers(cflags_game_eh)

cflags_sdk = [
    *cflags_base,
    "-fp_contract off",
    "-use_lmw_stmw on",
    "-str reuse,noreadonly",
    "-common off",
]

# Separate SDK headers avoid the same case-insensitive path collisions.
cflags_sdk_2004 = [
    *cflags_sdk[: cflags_sdk.index("-i include")],
    "-i include/sdk",
    "-i include/sdk/dolphin",
    "-i include/sdk/msl/c",
    "-i include/sdk/msl/c_math",
    "-i include/sdk/msl/c_ppc",
    "-i include/sdk/msl/cpp",
    "-ir include/sdk/private",
    *cflags_sdk[cflags_sdk.index("-i include") :],
    "-char unsigned",
    "-DSDK_REVISION=2",
    "-DVERSION=0",  # Upstream GameCube US define.
    "-D__GEKKO__",
]

# Metrowerks library flags
cflags_runtime = [
    *cflags_base,
    "-use_lmw_stmw on",
    "-str reuse,readonly",
    "-gccinc",
    "-common off",
    "-inline deferred",
]

cflags_trk = [
    *cflags_runtime,
    "-sdata 0",
    "-sdata2 0",
]

config.linker_version = "GC/2.6"


# Helper function for Dolphin libraries
def DolphinLib(lib_name: str, objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "mw_version": "GC/1.2.5n",
        "cflags": cflags_sdk,
        "padding": "small_data",
        "progress_category": "sdk",
        "objects": objects,
    }


# Helper function for REL script objects
def Rel(lib_name: str, objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "mw_version": "GC/2.6",
        "cflags": cflags_rel,
        "progress_category": "rels",
        "objects": objects,
    }


US = "GGTE01"
JP = "GGTJ01_01"
PAL = "GGTP01"

Matching = True                   # Object matches and should be linked
NonMatching = False               # Object does not match and should not be linked
Equivalent = config.non_matching  # Object should be linked when configured with --non-matching


# Object is only matching for specific versions
def MatchingFor(*versions):
    return config.version in versions


def EquivalentFor(*versions):
    return config.non_matching and config.version in versions


config.warn_missing_config = False
config.warn_missing_source = False
config.libs = [
    {
        "lib": "game part 1",
        "mw_version": "GC/2.6",
        "cflags": cflags_game,
        "progress_category": "game",
        "objects": [
            Object(NonMatching, "game/game.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gamecgschan.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gamecpadone.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gameccamera.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gamecstopwatch.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gametitle.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gamemenuex.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gamemenucursor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gamecfile.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gamecgenerator.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gamecobjarchive.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gamecmsg.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gamemsgcursor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gameccameraactions.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gameinterval.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/stagemicrophone.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/micaudio.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gamemicrophone.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gamemovie.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cgame.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gameobjupdate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gamexobjshadow.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gamexobjdraw.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gamemirrorfloor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gameclightset.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gameccameraset.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gamereset.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gameupdate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gamecamarea.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gameobjsetup.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gamecoordinfo.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gamecounter.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gamestart.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gameccameraview.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gameinit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gameptcllink.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gamefbsubwincam.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gameccameraproj.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gamefbsubwin.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gamefbsubwinfade.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gamefbmirror.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gamemain.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gameosreport.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cinterpolationnone.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/ccameraactionareaexm.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cinterfacecameraaction.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cinterfacevectorinterpolationdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cinterpolationeaseoutdivide.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cinterpolationlinearupdate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cjobj.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/attrinfodtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/ccolordtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cshadowdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cgsscreendtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cprimitivedtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cpadexdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cstatus64dtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cscrnwipecircledtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cfboutlinedtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cfbtexeffvirt.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cstatusdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/logodtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjsdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cevtdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/csounddtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xsounddtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/memcarddtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cmoviedtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gamecbaselight.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cthread.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/fbmirrorexpool.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/fbmirrorexdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/arcacheinfodtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cgamector.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cspritedtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cmsgcursorctor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cursorctor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjctor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cjanimdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cjarchivedtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cxobjintpctor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cobjarchivector.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cgeneratorctor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/menuexctor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/titlector.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/clightctor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cscrnwipedtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/ctpldtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cfilearchivedtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cbasector.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/ccameractor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cgsctor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/ctiledtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cgschannelctor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/fbmirrorex.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cpaddtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/progressive.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cfiletextdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gxa.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/sound.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gxb.cpp", cflags=with_hsd_headers(cflags_game), extra_cflags=["-inline noauto"]),
            Object(NonMatching, "game/stg.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/menu_cleanup.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/infoscrn.cpp", cflags=with_hsd_headers(cflags_game), extra_cflags=["-inline noauto,deferred"]),
            Object(NonMatching, "game/stage_data.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cjanim.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjfloor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/obja.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjwalk.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjinit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjinitialize.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjmapdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/ctrigondtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/obja_map.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/drsmenudtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/octamenureset.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/efftrail.cpp", cflags=with_hsd_headers(cflags_game)),
            *([Object(Matching, "game/xobjmatcolor.cpp", cflags=with_hsd_headers(cflags_game))]
              if config.version != JP else []),
            Object(NonMatching, "game/obja_chr.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xactradar.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xacttumble.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/objb_act.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/objb_attach.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjradarinit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjpopper.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/objb_plug.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjplugrelease.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjplugload.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/objb_sound.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjcoin.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjletickerdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/objb_leticker.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/objb_army.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjspider.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjspidersmall.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjspiderbossdraw.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactorstop.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjspiderbossreset.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/memcardrestore.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/memcardcapture.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/memcardbanner.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjchrjointrot.cpp", cflags=with_hsd_headers(cflags_game)),
            *([Object(Matching, "game/oxmenumarkdtor.cpp", cflags=with_hsd_headers(cflags_game))]
              if config.version != JP else []),
            Object(NonMatching, "game/cgameinit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactor3dtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjactor3ptcl.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactor3panel.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjactor3draw.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactor3update.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjactor3release.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/scrc.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjactortoystate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactortoydtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjactortoycmd.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactortoy.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjactortoydraw.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactortoyread.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactorjenny.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactormama.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjactormamaupdate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/mamadrop.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/mamareset.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactormamahide.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactorpapa.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactortao.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactordeca.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(MatchingFor(US, PAL), "game/ccameraactionareaexinit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/infopointinit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/memcardinit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjflagsinit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjplugcordpointpool.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjactordtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjreadanmdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/ccameraactionareaexdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/infopointdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/atcmenudtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xpaddtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cptcldtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjplugcordpointctor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/memcardcamera.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjspidersmallinit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjflagsdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjcoindtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjfreedtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjspidersmallstatus.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjspiderbossdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xactpoppershotdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjplugdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjwireeffdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjcopterdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjattachmentdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjpopperdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjradardtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjoxdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjcameradtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjplugcordnodedtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjlightdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/oxmenudtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjhboxdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjplugcordpointdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjplugcorddraw.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjplugcord.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xactioninit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjspiderbossstop.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjdustevent.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/memcardheader.cpp", cflags=cflags_game),
            Object(NonMatching, "game/memcardload.cpp", cflags=cflags_game),
            Object(NonMatching, "game/memcardscan.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjchratcanim.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjspiderbossstate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjflagsclear.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjcoinparam.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/titlestates.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjspiderbossupdate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjspiderbossinit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactor3init.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjspiderbossgetup.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjplugnodedraw.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjfloorheight.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjplugcordreset.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/memcardpos.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjcoinfall.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjdustpad.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjplugnode.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjchritem.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/fbsubwindraw.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(MatchingFor(JP), "game/msgdataformat.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/memcardreadstate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/memcardwritestate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjcurrentedge.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjchrplacement.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjchrresetstate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/plugcordcollision.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjfreeupdate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjcoinupdate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjspiderbossattack.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactorground.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjchrsuit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/memcardcheck.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjchrrestore.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjchrreset.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjplugnodes.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjdust.cpp", cflags=with_hsd_headers(cflags_game), extra_cflags=["-inline noauto"]),
            Object(NonMatching, "game/xobjfree.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjdustbillboard.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/memcardformat.cpp", cflags=cflags_game, extra_cflags=["-inline noauto"]),
            Object(NonMatching, "game/xobjfreeevent.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjreadwait.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/memcardread.cpp", cflags=cflags_game, extra_cflags=["-inline noauto"]),
            Object(NonMatching, "game/xobjspiderbosshit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/memcardscanjob.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjspiderbossse.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjspiderbossstates34.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjchrstop.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjspiderbossload.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjplugcordend.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactorupdate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/memcardfile.cpp", cflags=cflags_game),
            Object(Matching, "game/xaction.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/params.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/inventory.cpp"),
            Object(NonMatching, "game/xobjcamera.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjlight.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactorlook.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/memcardjob.cpp"),
            Object(Matching, "game/xobjactorse.cpp", cflags=with_hsd_headers(cflags_game), extra_cflags=["-inline auto,deferred"]),
            Object(NonMatching, "game/xobjhboxpart.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjhbox.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cinterpolationeaseout.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/ccameraactionareaex.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/evt.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/msg.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjs.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/ccameraactionarea.cpp", cflags=with_hsd_headers(cflags_game), extra_cflags=["-inline noauto"]),
            Object(NonMatching, "game/ccameraactionmap2.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/ccameraactionchase.cpp", cflags=with_hsd_headers(cflags_game), extra_cflags=["-inline auto,deferred"]),
            Object(Matching, "game/ccameraactionevt.cpp", cflags=with_hsd_headers(cflags_game), extra_cflags=["-inline auto,deferred"], padding="small_data+rodata"),
            Object(NonMatching, "game/ccameraactionscroll2.cpp", cflags=with_hsd_headers(cflags_game), extra_cflags=["-inline auto,deferred"]),
            Object(NonMatching, "game/ccameraactioneye.cpp", cflags=with_hsd_headers(cflags_game), extra_cflags=["-inline auto,deferred"]),
            Object(Matching, "game/ccameraactionanim.cpp", cflags=with_hsd_headers(cflags_game), extra_cflags=["-inline auto,deferred"]),
            Object(NonMatching, "game/ccameraactionreset.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/ccameraactionscroll.cpp", cflags=with_hsd_headers(cflags_game), extra_cflags=["-inline auto,deferred"]),
            Object(NonMatching, "game/cmotioncontrolcamera.cpp", cflags=with_hsd_headers(cflags_game), extra_cflags=["-inline noauto,deferred"]),
        ],
    },
    {
        "lib": "TRK_MINNOW_DOLPHIN",
        "mw_version": "GC/2.6",
        "cflags": cflags_trk,
        "padding": "small_sections",
        "progress_category": "runtime",
        "objects": [
            Object(Matching, "TRK_MINNOW_DOLPHIN/mainloop.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/nubevent.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/nubinit.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/msg.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/msgbuf.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/serpoll.c", extra_cflags=["-sdata 8"]),
            Object(Matching, "TRK_MINNOW_DOLPHIN/usr_put.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/dispatch.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/msghndlr.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/support.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/mutex_TRK.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/notify.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/flush_cache.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/mem_TRK.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/targimpl.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/targsupp.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/mpc_7xx_603e.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/__exception.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/dolphin_trk.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/main_TRK.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/dolphin_trk_glue.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/targcont.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/target_options.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/mslsupp.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/UDP_Stubs.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/ddh/main.c", extra_cflags=["-sdata 8"]),
            Object(Matching, "TRK_MINNOW_DOLPHIN/CircleBuffer.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/gdev/main.c", extra_cflags=["-sdata 8"]),
            Object(Matching, "TRK_MINNOW_DOLPHIN/MWTrace.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/MWCriticalSection_gc.cpp"),
        ],
    },
    {
        "lib": "Runtime.PPCEABI.H",
        "mw_version": "GC/2.6",
        "cflags": cflags_runtime,
        "padding": "small_sections",
        "progress_category": "runtime",
        "objects": [
            Object(Matching, "Runtime.PPCEABI.H/__va_arg.c"),
            Object(Matching, "Runtime.PPCEABI.H/global_destructor_chain.c"),
            Object(Matching, "Runtime.PPCEABI.H/delete.cpp", extra_cflags=["-Cpp_exceptions on"]),
            Object(Matching, "Runtime.PPCEABI.H/NMWException.cp", extra_cflags=["-Cpp_exceptions on"]),
            Object(Matching, "Runtime.PPCEABI.H/ptmf.c"),
            Object(Matching, "Runtime.PPCEABI.H/runtime.c"),
            Object(Matching, "Runtime.PPCEABI.H/__init_cpp_exceptions.cpp"),
            # Retail has 0x0255 in __unexpected's exception-table padding.
            Object(Matching, "Runtime.PPCEABI.H/Gecko_ExceptionPPC.cp", extra_cflags=["-Cpp_exceptions on", "-RTTI on"], extab_padding=[0x02, 0x55]),
            Object(Matching, "Runtime.PPCEABI.H/GCN_mem_alloc.c"),
            Object(Matching, "Runtime.PPCEABI.H/__mem.c"),
        ],
    },
    # RELs need a separate copy without small-data addressing.
    {
        "lib": "Runtime.PPCEABI.H rels",
        "mw_version": "GC/2.6",
        "cflags": [*cflags_runtime, "-sdata 0", "-sdata2 0"],
        "progress_category": "runtime",
        "rels": True,
        "objects": [
            Object(Matching, "Runtime.PPCEABI.H/global_destructor_chain.c"),
        ],
    },
    {
        "lib": "MSL_C.PPCEABI.bare.H",
        "mw_version": "GC/2.6",
        "cflags": cflags_runtime,
        "padding": "small_sections",
        "progress_category": "runtime",
        "objects": [
            Object(Matching, "MSL_C.PPCEABI.bare.H/PPC_EABI/abort_exit.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common/alloc.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common/ansi_files.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/ansi_fp.c", extra_cflags=["-str pool"]),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common/buffer_io.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/PPC_EABI/critical_regions.gamecube.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common/ctype.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common/direct_io.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common/file_io.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common/FILE_POS.C"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common/mbstring.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common/mem.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common/mem_funcs.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common/misc_io.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common/printf.c", extra_cflags=["-str pool"]),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common/qsort.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common/rand.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common/scanf.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common/signal.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common/string.c", extra_cflags=["-str pool"]),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common/strtoul.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common/wchar_io.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/PPC_EABI/uart_console_io_gcn.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/e_acos.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/e_asin.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/e_atan2.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/e_exp.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/e_fmod.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/e_log.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/e_pow.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/e_rem_pio2.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/k_cos.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/k_rem_pio2.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/k_sin.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/k_tan.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/s_atan.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/s_copysign.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/s_cos.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/s_floor.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/s_frexp.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/s_ldexp.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/s_modf.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/s_sin.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/s_tan.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/w_acos.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/w_asin.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/w_atan2.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/w_exp.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/w_fmod.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/w_log.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/w_pow.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/e_sqrt.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/PPC_EABI/math_ppc.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/w_sqrt.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common/errno.c"),
            Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common/float.c"),
        ],
    },
    DolphinLib(
        "OdemuExi2",
        [
            Object(Matching, "dolphin/OdemuExi2/DebuggerDriver.c", extra_cflags=["-inline deferred"]),
        ],
    ),
    DolphinLib(
        "exi",
        [
            Object(Matching, "dolphin/exi/EXIBios.c"),
            Object(Matching, "dolphin/exi/EXIUart.c", cflags=cflags_sdk_2004),
        ],
    ),
    DolphinLib(
        "si",
        [
            Object(Matching, "dolphin/si/SIBios.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/si/SISamplingRate.c"),
        ],
    ),
    DolphinLib(
        "base",
        [
            Object(Matching, "dolphin/base/PPCArch.c"),
        ],
    ),
    DolphinLib(
        "db",
        [
            Object(Matching, "dolphin/db/db.c"),
        ],
    ),
    DolphinLib(
        "os",
        [
            Object(Matching, "dolphin/os/OS.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/os/OSAlarm.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/os/OSAlloc.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/os/OSArena.c"),
            Object(Matching, "dolphin/os/OSAudioSystem.c"),
            Object(Matching, "dolphin/os/OSCache.c"),
            Object(Matching, "dolphin/os/OSContext.c"),
            Object(Matching, "dolphin/os/OSError.c"),
            Object(Matching, "dolphin/os/OSExec.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/os/OSFatal.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/os/OSFont.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/os/OSInterrupt.c"),
            Object(Matching, "dolphin/os/OSLink.c", cflags=cflags_sdk_2004, padding="small_data+data"),
            Object(Matching, "dolphin/os/OSMessage.c"),
            Object(Matching, "dolphin/os/OSMemory.c"),
            Object(Matching, "dolphin/os/OSMutex.c"),
            Object(Matching, "dolphin/os/OSReboot.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/os/OSReset.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/os/OSResetSW.c"),
            Object(Matching, "dolphin/os/OSRtc.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/os/OSStopwatch.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/os/OSSync.c"),
            Object(Matching, "dolphin/os/OSThread.c"),
            Object(Matching, "dolphin/os/OSTime.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/os/__ppc_eabi_init.cpp"),
            Object(Matching, "dolphin/os/__start.c"),
        ],
    ),
    DolphinLib(
        "mtx",
        [
            Object(Matching, "dolphin/mtx/mtx.c"),
            Object(Matching, "dolphin/mtx/mtxvec.c"),
            Object(Matching, "dolphin/mtx/mtx44.c"),
            Object(Matching, "dolphin/mtx/vec.c"),
        ],
    ),
    DolphinLib(
        "dvd",
        [
            Object(Matching, "dolphin/dvd/dvdlow.c"),
            Object(Matching, "dolphin/dvd/dvdfs.c"),
            Object(Matching, "dolphin/dvd/dvd.c", cflags=cflags_sdk_2004, extra_cflags=["-char signed"]),
            Object(Matching, "dolphin/dvd/dvdqueue.c"),
            Object(Matching, "dolphin/dvd/dvderror.c"),
            Object(Matching, "dolphin/dvd/dvdidutils.c"),
            Object(Matching, "dolphin/dvd/dvdFatal.c", cflags=cflags_sdk_2004, extra_cflags=["-char signed"]),
            Object(Matching, "dolphin/dvd/fstload.c"),
        ],
    ),
    DolphinLib(
        "vi",
        [
            Object(Matching, "dolphin/vi/vi.c", cflags=cflags_sdk_2004),
        ],
    ),
    DolphinLib(
        "pad",
        [
            Object(Matching, "dolphin/pad/Pad.c"),
        ],
    ),
    DolphinLib(
        "ai",
        [
            Object(Matching, "dolphin/ai/ai.c"),
        ],
    ),
    DolphinLib(
        "ar",
        [
            Object(Matching, "dolphin/ar/ar.c"),
            Object(Matching, "dolphin/ar/arq.c"),
        ],
    ),
    DolphinLib(
        "ax",
        [
            Object(Matching, "dolphin/ax/AX.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/ax/AXAlloc.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/ax/AXAux.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/ax/AXCL.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/ax/AXOut.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/ax/AXSPB.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/ax/AXVPB.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/ax/AXComp.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/ax/DSPCode.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/ax/AXProf.c", cflags=cflags_sdk_2004),
        ],
    ),
    DolphinLib(
        "axfx",
        [
            Object(Matching, "dolphin/axfx/reverb_hi.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/axfx/reverb_std.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/axfx/chorus.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/axfx/delay.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/axfx/axfx.c", cflags=cflags_sdk_2004),
        ],
    ),
    DolphinLib(
        "dsp",
        [
            Object(Matching, "dolphin/dsp/dsp.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/dsp/dsp_debug.c"),
            Object(Matching, "dolphin/dsp/dsp_task.c", cflags=cflags_sdk_2004),
        ],
    ),
    DolphinLib(
        "card",
        [
            Object(Matching, "dolphin/card/CARDBios.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/card/CARDUnlock.c"),
            Object(Matching, "dolphin/card/CARDRdwr.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/card/CARDBlock.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/card/CARDDir.c"),
            Object(Matching, "dolphin/card/CARDCheck.c"),
            Object(Matching, "dolphin/card/CARDMount.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/card/CARDFormat.c"),
            Object(Matching, "dolphin/card/CARDOpen.c"),
            Object(Matching, "dolphin/card/CARDCreate.c"),
            Object(Matching, "dolphin/card/CARDRead.c"),
            Object(Matching, "dolphin/card/CARDWrite.c"),
            Object(Matching, "dolphin/card/CARDDelete.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/card/CARDStat.c"),
            Object(Matching, "dolphin/card/CARDNet.c", cflags=cflags_sdk_2004),
        ],
    ),
    DolphinLib(
        "gx",
        [
            Object(Matching, "dolphin/gx/GXInit.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/gx/GXFifo.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/gx/GXAttr.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/gx/GXMisc.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/gx/GXGeometry.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/gx/GXFrameBuf.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/gx/GXLight.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/gx/GXTexture.c"),
            Object(Matching, "dolphin/gx/GXBump.c"),
            Object(Matching, "dolphin/gx/GXTev.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/gx/GXPixel.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/gx/GXDraw.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/gx/GXDisplayList.c"),
            Object(Matching, "dolphin/gx/GXTransform.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/gx/GXPerf.c"),
        ],
    ),
    DolphinLib(
        "arc",
        [
            Object(Matching, "dolphin/arc/arc.c"),
        ],
    ),
    DolphinLib(
        "amcstubs",
        [
            Object(Matching, "dolphin/amcstubs/AmcExi2Stubs.c"),
        ],
    ),
    DolphinLib(
        "odenotstub",
        [
            Object(Matching, "dolphin/odenotstub/odenotstub.c"),
        ],
    ),
    {
        "lib": "game part 2",
        "mw_version": "GC/2.6",
        "padding": "small_data",
        "cflags": cflags_game_eh,
        "progress_category": "game",
        "objects": [
            Object(NonMatching, "game/ut.cpp", cflags=cflags_hsd_game, extra_cflags=["-inline auto,deferred"]),
            Object(NonMatching, "game/mem.cpp"),
            Object(NonMatching, "game/ccamera.cpp", cflags=cflags_hsd_game, extra_cflags=["-inline auto,deferred"]),
            Object(Matching, "game/clight.cpp", cflags=cflags_hsd_game, extra_cflags=["-inline auto,deferred"]),
            Object(Matching, "game/cshadow.cpp", cflags=cflags_hsd_game, extra_cflags=["-inline auto,deferred"]),
            Object(NonMatching, "game/cgs.cpp", cflags=cflags_hsd_game, extra_cflags=["-inline auto,deferred"]),
            Object(NonMatching, "game/cprimitive.cpp", extra_cflags=["-inline auto,deferred"]),
            Object(NonMatching, "game/cpad.cpp", cflags=cflags_hsd_game),
            Object(NonMatching, "game/cfont.cpp"),
            Object(Matching, "game/cfile.cpp", extra_cflags=["-inline auto,deferred"], padding="small_data"),
            Object(Matching, "game/ctpl.cpp", extra_cflags=["-inline auto,deferred"]),
            Object(NonMatching, "game/cobj.cpp", cflags=cflags_hsd_game, extra_cflags=["-inline auto,deferred"]),
            Object(NonMatching, "game/ctrigon.cpp", cflags=cflags_hsd_game, extra_cflags=["-inline auto,deferred"]),
            Object(Matching, "game/ctoon.cpp", cflags=cflags_hsd_game, extra_cflags=["-inline auto,deferred"]),
            Object(NonMatching, "game/fbeffect.cpp", extra_cflags=["-inline auto,deferred"]),
            Object(Matching, "game/cparticle.cpp", cflags=cflags_hsd_game),
            Object(Matching, "game/cmodule.cpp"),
            Object(Matching, "game/ccard.cpp"),
            Object(Matching, "game/cmovie.cpp", extra_cflags=["-inline auto,deferred"]),
        ],
    },
    {
        "lib": "sysdolphin (game built)",
        "mw_version": "GC/2.6",
        "cflags": cflags_hsd_game,
        "padding": "small_data",
        "progress_category": "hsd",
        "objects": [
            Object(Matching, "sysdolphin/baselib/state.c", extra_cflags=["-inline auto,deferred"]),
            Object(Matching, "sysdolphin/baselib/shadow.c", extra_cflags=["-inline auto,deferred"]),
            Object(NonMatching, "sysdolphin/baselib/pobj.c", extra_cflags=["-inline auto,deferred"]),
            Object(Matching, "sysdolphin/baselib/tobj.c", extra_cflags=["-inline auto,deferred"], padding="small_data+rodata"),
            Object(NonMatching, "sysdolphin/baselib/psdisp.c", extra_cflags=["-inline auto,deferred"]),
        ],
    },
    {
        "lib": "thp player (game built)",
        "mw_version": "GC/2.6",
        "cflags": cflags_game_eh,
        "padding": "small_data",
        "progress_category": "game",
        "objects": [
            Object(Matching, "game/THPSimple.cpp", extra_cflags=["-inline auto,deferred"]),
            Object(Matching, "game/THPDraw.cpp", extra_cflags=["-inline auto,deferred"]),
            Object(NonMatching, "game/xobjhboxupdate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/plugcordsimulation.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjchrsuitjoints.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjspiderbossevent.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjchrdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactorjointupdate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/plugcordpoints.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjspiderbossplacement.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/plugcordpush.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjchrattachmentchange.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactorevent.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjchrdisplay.cpp", cflags=with_hsd_headers(cflags_game)),
        ],
    },
    {
        "lib": "sysdolphin",
        "mw_version": "GC/2.6",
        "cflags": cflags_hsd,
        "padding": "small_data",
        "progress_category": "hsd",
        "objects": [
            Object(Matching, "sysdolphin/baselib/dobj.c"),
            Object(Matching, "sysdolphin/baselib/tev.c"),
            Object(Matching, "sysdolphin/baselib/mobj.c"),
            Object(Matching, "sysdolphin/baselib/aobj.c", padding="small_data+data"),
            Object(NonMatching, "sysdolphin/baselib/lobj.c"),
            Object(Matching, "sysdolphin/baselib/cobj.c"),
            Object(Matching, "sysdolphin/baselib/fobj.c", padding="small_data+data"),
            Object(NonMatching, "sysdolphin/baselib/jobj.c"),
            Object(Matching, "sysdolphin/baselib/displayfunc.c", padding="small_data+data"),
            Object(Matching, "sysdolphin/baselib/initialize.c"),
            Object(NonMatching, "sysdolphin/baselib/video.c"),
            Object(Matching, "sysdolphin/baselib/controller.c"),
            Object(Matching, "sysdolphin/baselib/rumble.c"),
            Object(Matching, "sysdolphin/baselib/spline.c"),
            Object(NonMatching, "sysdolphin/baselib/mtx.c"),
            Object(Matching, "sysdolphin/baselib/util.c"),
            Object(Matching, "sysdolphin/baselib/objalloc.c"),
            Object(Matching, "sysdolphin/baselib/robj.c"),
            Object(Matching, "sysdolphin/baselib/id.c"),
            Object(Matching, "sysdolphin/baselib/wobj.c"),
            Object(Matching, "sysdolphin/baselib/fog.c"),
            Object(Matching, "sysdolphin/baselib/perf.c"),
            Object(Matching, "sysdolphin/baselib/list.c"),
            Object(Matching, "sysdolphin/baselib/object.c"),
            Object(Matching, "sysdolphin/baselib/quatlib.c"),
            Object(Matching, "sysdolphin/baselib/memory.c", padding="small_data+data"),
            Object(Matching, "sysdolphin/baselib/archive.c"),
            Object(Matching, "sysdolphin/baselib/random.c"),
            Object(Matching, "sysdolphin/baselib/bytecode.c"),
            Object(Matching, "sysdolphin/baselib/class.c"),
            Object(Matching, "sysdolphin/baselib/hash.c"),
            Object(NonMatching, "sysdolphin/baselib/texp.c"),
            Object(Matching, "sysdolphin/baselib/texpdag.c"),
            Object(Matching, "sysdolphin/baselib/texpopt.c"),
            Object(Matching, "sysdolphin/baselib/debug.c", padding="small_data+data"),
            Object(Matching, "sysdolphin/baselib/particle.c"),
            Object(Matching, "sysdolphin/baselib/psinterpret.c", padding="small_data+data"),
            Object(NonMatching, "sysdolphin/baselib/generator.c", padding="small_data+data"),
            Object(Matching, "sysdolphin/baselib/psdisptev.c"),
            Object(Matching, "sysdolphin/baselib/psappsrt.c"),
            Object(Matching, "sysdolphin/baselib/pslist.c"),
            Object(NonMatching, "sysdolphin/baselib/axdriver.c"),
            Object(Matching, "sysdolphin/baselib/devcom.c"),
            Object(Matching, "sysdolphin/baselib/dvdsche.c"),
            Object(NonMatching, "sysdolphin/baselib/synth.c"),
        ],
    },
    DolphinLib(
        "mic",
        [
            Object(Matching, "dolphin/mic/mic.c"),
            Object(Matching, "dolphin/mic/m2s.c"),
        ],
    ),
    DolphinLib(
        "thp",
        [
            Object(Matching, "dolphin/thp/THPDec.c"),
            Object(Matching, "dolphin/thp/THPAudio.c"),
        ],
    ),
    Rel(
        "rel",
        [
            Object(Matching, "rel/module.c"),
        ],
    ),
    {
        "lib": "rel game",
        "mw_version": "GC/2.6",
        "cflags": with_hsd_headers(cflags_rel),
        "progress_category": "rels",
        "objects": [
            Object(Matching, "rel/ahk/ahk.cpp"),
            Object(Matching, "rel/hori/hori.cpp"),
            Object(NonMatching, "rel/shun/shun.cpp"),
            Object(Matching, "rel/fafa.cpp"),
            Object(NonMatching, "rel/stage01/hamburg.cpp"),
            Object(Matching, "rel/ana.cpp", extra_cflags=["-inline auto,deferred"]),
            Object(NonMatching, "rel/stage02/fighter.cpp"),
            Object(MatchingFor(US, JP), "rel/stage02/rouka.cpp", extra_cflags=["-inline auto,deferred"]),
            Object(Matching, "rel/stage02/car.cpp", extra_cflags=["-inline auto,deferred"]),
            Object(NonMatching, "rel/stage03/decapanel.cpp"),
            Object(Matching, "rel/stage04/jenny.cpp"),
            Object(NonMatching, "rel/trash.cpp"),
            Object(Matching, "rel/stage06/parent.cpp"),
            Object(NonMatching, "rel/stage07/living.cpp"),
            Object(Matching, "rel/stage08/stage08.cpp"),
            Object(Matching, "rel/stage09/ambient.cpp"),
            Object(Matching, "rel/stage11/haisui.cpp"),
            Object(Matching, "rel/stage16/ufo.cpp"),
            Object(Matching, "rel/stage17/stage17.cpp"),
            Object(Matching, "rel/stage18/kako.cpp"),
            Object(Matching, "rel/stage20/stage20.cpp"),
        ],
    },
]


# Optional callback to adjust link order. This can be used to add, remove, or reorder objects.
# This is called once per module, with the module ID and the current link order.
#
# For example, this adds "dummy.c" to the end of the DOL link order if configured with --non-matching.
# "dummy.c" *must* be configured as a Matching (or Equivalent) object in order to be linked.
def link_order_callback(module_id: int, objects: List[str]) -> List[str]:
    # Don't modify the link order for matching builds
    if not config.non_matching:
        return objects
    if module_id == 0:  # DOL
        return objects + ["dummy.c"]
    return objects


# Uncomment to enable the link order callback.
# config.link_order_callback = link_order_callback


# Optional extra categories for progress tracking
# Adjust as desired for your project
config.progress_categories = [
    ProgressCategory("game", "Game Code"),
    ProgressCategory("sdk", "SDK Code"),
    ProgressCategory("hsd", "HSD Code"),
    ProgressCategory("runtime", "Runtime Code"),
]
# Empty categories report 100%.
if any(lib.get("progress_category") == "rels" for lib in config.libs):
    config.progress_categories.append(ProgressCategory("rels", "REL Code"))
config.progress_each_module = args.verbose
config.progress_use_fancy = True
config.progress_code_fancy_frac = 1000
config.progress_code_fancy_item = "Happy Points"
config.progress_data_fancy_frac = 100
config.progress_data_fancy_item = "Utilibots"
# Optional extra arguments to `objdiff-cli report generate`
config.progress_report_args = [
    # Marks relocations as mismatching if the target value is different
    # Default is "functionRelocDiffs=none", which is most lenient
    # "--config functionRelocDiffs=data_value",
]


def append_treemap_step(config: ProjectConfig) -> None:
    # Reports are added after custom project steps.
    report = (config.out_path() / "report.json").as_posix()
    page = (config.build_dir / "treemap.html").as_posix()
    with open("build.ninja", "a", encoding="utf-8") as f:
        f.write(
            "\n# Local treemap page\n"
            "rule treemap\n"
            f"  command = $python tools/treemap.py --version {config.version} --report $in -o $out\n"
            "  description = TREEMAP $out\n"
            f"build {page}: treemap {report} | tools/treemap.py tools/treemap_layout.py tools/treemap_template.html\n"
            f"default {page}\n"
        )


if args.mode == "configure":
    # Write build.ninja and objdiff.json
    generate_build(config)
    append_treemap_step(config)
elif args.mode == "progress":
    # Print progress information
    calculate_progress(config)
else:
    sys.exit("Unknown mode: " + args.mode)
