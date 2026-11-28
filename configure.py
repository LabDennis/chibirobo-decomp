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
import json
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
from tools.versions import CONFIGURE_ARGS_PREFIX

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
config.reconfig_deps = [Path("tools/versions.py")]

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

Matching = True                   # Link the verified source object
NonMatching = False               # Use the extracted object in the matching build
Equivalent = config.non_matching  # Object should be linked when configured with --non-matching


# Object is only matching for specific versions
def MatchingFor(*versions):
    return config.version in versions


def EquivalentFor(*versions):
    return config.non_matching and config.version in versions


config.warn_missing_config = False
config.warn_missing_source = False
# Game units with hand-written vtable stores or compiler-generated lifetime
# helpers use extracted objects until those routines are expressed in C++.
config.libs = [
    {
        "lib": "game part 1",
        "mw_version": "GC/2.6",
        "cflags": cflags_game,
        "padding": "small_sections",
        "progress_category": "game",
        "objects": [
            Object(NonMatching, "game/game.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gameccamera.cpp", source="game/cbasector.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gamecstopwatch.cpp", source="game/cbasector.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gamecpadone.cpp", source="game/cbasector.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/gametitle.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/gamemenuex.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/gamemenucursor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gamecfile.cpp", source="game/cobjarchivector.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gamecgenerator.cpp", source="game/logodtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gamecobjarchive.cpp", source="game/cobjarchivector.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/gamecmsg.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/gameccameraactions.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gameccameraactions_tail.cpp"),
            Object(NonMatching, "game/cmotioncontrolcamera_vtable.cpp"),
            Object(NonMatching, "game/ccameraactions_vtable.cpp"),
            Object(NonMatching, "game/ccameraactionarea_vtable.cpp"),
            Object(NonMatching, "game/ccameraactionmap_vtable.cpp"),
            Object(NonMatching, "game/ccameraactionreset_vtable.cpp"),
            Object(Matching, "game/gameinterval.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/stagemicrophone.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/micaudio.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gamemicrophone.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/gamemovie.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/cgame.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gameobjupdate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/gamexobjshadow.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gamexobjdraw.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gamemirrorfloor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gameclightset.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/gameccameraset.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gamereset.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/gameupdate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cmenuopen.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/gamedrawlists.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gamecamarea.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gameobjsetup.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gamecoordinfo.cpp", source="game/xobjfreedtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gamecounter.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gamestart.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gameccameraview.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gameinit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gameptcllink.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gamefbsubwincam.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gameccameraproj.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gamefbsubwin.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/gamefbsubwinfade.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gamefbmirror.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/gamemain.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gameosreport.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/cinterpolationnone.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cinterpolationnone_vtable.cpp"),
            Object(NonMatching, "game/cvectornoneinterpolation_vtable.cpp"),
            Object(Equivalent, "game/ccameraactionareaexmdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/ccameraactionareaexm.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/cinterfacecameraaction.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cinterfacevectorinterpolationdtor.cpp", source="game/cinterfacecameraaction.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cinterfacevectorinterpolation_rtti.cpp"),
            Object(NonMatching, "game/cinterfacecameraaction_vtables.cpp"),
            Object(NonMatching, "game/cinterpolationlinear_vtable.cpp"),
            Object(Equivalent, "game/cinterpolationeaseoutdivide.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cvectoreaseoutdivideinterpolation_vtable.cpp"),
            Object(NonMatching, "game/cinterpolationeaseoutdivide_vtable.cpp"),
            Object(Matching, "game/cinterpolationlinearupdate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/cinterfaceactiondtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/ccameramementodtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/cmsgdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/cjobj.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/attrinfodtor.cpp", source="game/cursorctor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/ccolordtor.cpp", source="game/cbasector.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cshadowdtor.cpp", source="game/cbasector.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cgsscreendtor.cpp", source="game/cgsctor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cscrnwipecircle_rtti.cpp"),
            Object(NonMatching, "game/cscrnwipe_rtti.cpp"),
            Object(NonMatching, "game/cfboutline_rtti.cpp"),
            Object(NonMatching, "game/cpadexdtor.cpp", source="game/cbasector.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cstatus64dtor.cpp", source="game/cbasector.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/cscrnwipecircledtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/cfboutlinedtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cfbtexeff_vtable.cpp"),
            Object(NonMatching, "game/fbsubwin_vtable.cpp"),
            Object(NonMatching, "game/fbmirror_vtable.cpp"),
            Object(Equivalent, "game/cfbtexeffvirt.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cstatusdtor.cpp", source="game/cgamector.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/logodtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjsdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cevtdtor.cpp", source="game/xobjfreedtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/csounddtor.cpp", source="game/cgamector.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xsounddtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/csound_vtable.cpp"),
            Object(NonMatching, "game/xsound_vtable.cpp"),
            Object(NonMatching, "game/cjointrot_vtable.cpp"),
            Object(NonMatching, "game/params_vtable.cpp"),
            Object(NonMatching, "game/xpad_vtable.cpp"),
            Object(NonMatching, "game/memcard_vtable.cpp"),
            Object(NonMatching, "game/cgenerator_vtable.cpp"),
            Object(Equivalent, "game/memcarddtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/cmoviedtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/gamecbaselight.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cthread_vtable.cpp"),
            Object(NonMatching, "game/arprecache_vtable.cpp"),
            Object(NonMatching, "game/arcacheinfo_vtable.cpp"),
            Object(NonMatching, "game/cmovie_vtable.cpp"),
            Object(NonMatching, "game/ctrigon_vtable.cpp"),
            Object(NonMatching, "game/xobjactor3_vtable.cpp"),
            Object(Equivalent, "game/cthread.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/fbmirrorexpool.cpp", source="game/cgameinit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/fbmirrorexdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/arcacheinfodtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cgamector.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cmsgcursorctor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cursorctor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjctor.cpp", source="game/cursorctor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/cjanimdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cjanim_vtable.cpp"),
            Object(Equivalent, "game/cjarchivedtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cxobjintpctor.cpp", source="game/cursorctor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cobjarchivector.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cvecctor.cpp", source="game/cgamector.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/cgeneratorctor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cfilector.cpp", source="game/cgamector.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/menuex_vtable.cpp"),
            Object(NonMatching, "game/menucursor_vtable.cpp"),
            Object(NonMatching, "game/cmsg_vtable.cpp"),
            Object(NonMatching, "game/drsmenu_vtable.cpp"),
            Object(NonMatching, "game/atcmenu_vtables.cpp"),
            Object(Matching, "game/menuexctor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/titlector.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/clightctor.cpp", source="game/cbasector.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/cscrnwipedtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cbasector.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cgsctor.cpp", cflags=with_hsd_headers(cflags_game)),
            # CGs generates these member constructors and destructors.
            Object(NonMatching, "game/cprimitivedtor.cpp", source="game/cgsctor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cspritedtor.cpp", source="game/cgsctor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/ctiledtor.cpp", source="game/cgsctor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cgschannelctor.cpp", source="game/cgsctor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gamecgschan.cpp", source="game/cgsctor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/fbmirrorex.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cpaddtor.cpp", source="game/fbmirrorex.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/progressive.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/cfiletextdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/gxa.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/sound.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/screenwipe.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/gxb.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/titlescreen.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/stg.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/menu_cleanup.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/infoscrn.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/stage_data.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/stage_defaults.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/stage_tables.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cjanim.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/cjanimarchive.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjfloor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/obja.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjmaterial.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjwalk.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjinit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjinitialize.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjmapdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/ctrigondtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/obja_map.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/drsmenudtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/octamenureset.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/efftrail.cpp", cflags=with_hsd_headers(cflags_game)),
            *([Object(Matching, "game/xobjmatcolor.cpp", cflags=with_hsd_headers(cflags_game))]
              if config.version != JP else []),
            Object(NonMatching, "game/obja_chr.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/playerframeevents.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/obja_chr_late.cpp", source="game/obja_chr.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xactradar.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xactpopper.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xactcopter.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xacttumbleanim.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xacttumble.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xactpushobject.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xactpush.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xactrope.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xactladderanim.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/objb_act_motion.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xacthang.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xactliftcontacts.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/objb_act.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xactliftanim.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/objb_act_helpers.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xactlift.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/objb_attach.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjradarinit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjpopper.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/objb_plug.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjplugrelease.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjplugload.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/objb_sound.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjcoin.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjletickerdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/objb_leticker.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjletickerview.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/objb_leticker_tail.cpp", source="game/objb_leticker.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/ccameraactionleticker.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/objb_army.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjspider.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjspidersmall.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjspidersmallstate2.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjspidersmall_tail.cpp", source="game/xobjspidersmall.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjspiderbossdraw.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjactorstop.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjactorconstants.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjspiderbossreset.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjspiderbossdata.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/memcardrestore.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(MatchingFor(JP, PAL) or Equivalent, "game/memcardcapture.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/memcardbanner.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjchrjointrot.cpp", cflags=with_hsd_headers(cflags_game)),
            *([Object(Matching, "game/oxmenumarkdtor.cpp", cflags=with_hsd_headers(cflags_game))]
              if config.version != JP else []),
            Object(NonMatching, "game/cgameinit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjactor3dtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjactor3ptcl.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactor3panel.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjactor3draw.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjactor3update.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjactor3release.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/scrc.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjactortoystate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactortoydtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjactortoycmd.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactortoy.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjactortoybehavior.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/cjobjattach.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjactortoydraw.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjactortoyread.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactorjenny.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactormama.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjactormamaupdate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/mamadrop.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/mamareset.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactormamahide.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactorpapa.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactortao.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactordeca.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(MatchingFor(US, PAL) or EquivalentFor(JP), "game/ccameraactionareaexinit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/infopointinit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/memcardinit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjflagsinit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjactordtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjreadanmdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/ccameraactionareaexdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/infopointdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/atcmenudtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xpaddtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cptcldtor.cpp", source="game/xactpoppershotdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjplugcordpointctor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/memcardcamera.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjspidersmallinit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjflagsdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjcoindtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjfreedtor.cpp", cflags=with_hsd_headers(cflags_game)),
            # The generated vtable still needs a split that preserves the class's section order.
            Object(NonMatching, "game/xobjspiderbossdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xactpoppershotdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjplugdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjattachmentdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjcameradtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjlightdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/oxmenudtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjhboxdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjplugcordpointdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjplugcorddraw.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjplugcord.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xactioninit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjspiderbossstop.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjdustevent.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/memcardheader.cpp", cflags=cflags_game),
            Object(Equivalent, "game/memcardload.cpp", cflags=cflags_game),
            Object(Equivalent, "game/memcardscan.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/memcardscan_format.cpp"),
            Object(NonMatching, "game/memcardscan_buffer.cpp"),
            Object(NonMatching, "game/memcardpreview_buffers.cpp"),
            Object(Equivalent, "game/xobjchratcanim.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/playerattachmentpaths.cpp"),
            Object(Equivalent, "game/xobjspiderbossstate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjflagsclear.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjcoinparam.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/titlestates.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjspiderbossupdate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjspiderbossinit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactor3init.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjspiderbossgetup.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjplugnodedraw.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjfloorheight.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjplugcordreset.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/memcardpos.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjcoinfall.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjdustpad.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjplugnode.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjchritem.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/fbsubwindraw.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(MatchingFor(JP, PAL) or Equivalent, "game/msgdataformat.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/memcardreadstate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/memcardwritestate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjcurrentedge.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjchrplacement.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjchrresetstate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/plugcordcollision.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjfreeupdate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjcoinupdate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjspiderbossattack.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjactorground.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjchrsuit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/memcardcheck.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjchrrestore.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjchrreset.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjplugnodes.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjdust.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjfree.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjdustbillboard.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/memcardformat.cpp", cflags=cflags_game),
            Object(NonMatching, "game/xobjfreeevent.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjreadwait.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/memcardread.cpp", cflags=cflags_game),
            Object(Equivalent, "game/xobjspiderbosshit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/memcardscanjob.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjspiderbossse.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjspiderbossconstants.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjspiderbossstates34.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjchrstop.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjspiderbossload.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjplugcordend.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjactorupdate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/memcardfile.cpp", cflags=cflags_game),
            Object(Matching, "game/xaction.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/params.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/inventory.cpp"),
            Object(Equivalent, "game/xobjcamera.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjlight.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactorlook.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/memcardjob.cpp"),
            Object(Matching, "game/xobjactorse.cpp", cflags=with_hsd_headers(cflags_game), extra_cflags=["-inline auto,deferred"]),
            Object(NonMatching, "game/xobjhboxpart.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjhbox.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/cinterpolationeaseout.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cinterpolationeaseout_dtors.cpp", source="game/gameccameraactions.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cinterpolationeaseout_rtti.cpp"),
            Object(Equivalent, "game/cameraareadebug.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/ccameraactionareaex.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/cameraareagroups.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/cameraareaentries.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/evt.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/evt_schedule.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/evt_init.cpp", source="game/evt.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/msg.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjs.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/ccameraactionarea.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/ccameraactionmap2.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/ccameraactionchase.cpp", cflags=with_hsd_headers(cflags_game), extra_cflags=["-inline auto,deferred"]),
            Object(Equivalent, "game/ccameraactionevt.cpp", cflags=with_hsd_headers(cflags_game), extra_cflags=["-inline auto,deferred"], padding="small_data+rodata"),
            Object(Equivalent, "game/ccameraactionscroll2.cpp", cflags=with_hsd_headers(cflags_game), extra_cflags=["-inline auto,deferred"]),
            Object(Equivalent, "game/ccameraactioneye.cpp", cflags=with_hsd_headers(cflags_game), extra_cflags=["-inline auto,deferred"]),
            Object(Matching, "game/ccameraactionanim.cpp", cflags=with_hsd_headers(cflags_game), extra_cflags=["-inline auto,deferred"]),
            Object(Equivalent, "game/ccameraactionreset.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/ccameraactionscroll.cpp", cflags=with_hsd_headers(cflags_game), extra_cflags=["-inline auto,deferred"]),
            Object(Equivalent, "game/cmotioncontrolcamera.cpp", cflags=with_hsd_headers(cflags_game)),
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
            # The C source prints characters literally through the variadic API.
            Object(Equivalent, "TRK_MINNOW_DOLPHIN/usr_put.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/dispatch.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/msghndlr.c"),
            Object(Equivalent, "TRK_MINNOW_DOLPHIN/support.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/mutex_TRK.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/notify.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/flush_cache.s", add_to_all=False),
            Object(Equivalent, "TRK_MINNOW_DOLPHIN/mem_TRK.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/trk_target_context.s", add_to_all=False,
                   extra_as_implicit=["include/sdk/private/trk/state.inc"]),
            # Splitting out the context code changes MWCC's BSS symbol order.
            Object(Equivalent, "TRK_MINNOW_DOLPHIN/targimpl.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/targsupp.s", add_to_all=False),
            Object(Matching, "TRK_MINNOW_DOLPHIN/mpc_7xx_603e.s", add_to_all=False,
                   extra_as_implicit=["include/sdk/private/trk/state.inc"]),
            Object(Matching, "TRK_MINNOW_DOLPHIN/__exception.s", add_to_all=False),
            Object(Matching, "TRK_MINNOW_DOLPHIN/trk_boot.s", add_to_all=False,
                   extra_as_implicit=["include/sdk/private/trk/state.inc"]),
            Object(Equivalent, "TRK_MINNOW_DOLPHIN/dolphin_trk.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/main_TRK.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/trk_context_load.s", add_to_all=False,
                   extra_as_implicit=["include/sdk/private/os/exception_context.inc"]),
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
            Object(Equivalent, "Runtime.PPCEABI.H/ptmf.c"),
            Object(Equivalent, "Runtime.PPCEABI.H/ptmf_call.s", add_to_all=False),
            Object(Equivalent, "Runtime.PPCEABI.H/runtime.s", add_to_all=False),
            Object(Equivalent, "Runtime.PPCEABI.H/get_sda2_base.s", add_to_all=False),
            Object(Matching, "Runtime.PPCEABI.H/__init_cpp_exceptions.cpp"),
            Object(Equivalent, "Runtime.PPCEABI.H/gecko_exception_context.s", add_to_all=False),
            Object(Equivalent, "Runtime.PPCEABI.H/Gecko_ExceptionPPC.cp", extra_cflags=["-Cpp_exceptions on", "-RTTI on"]),
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
            Object(Matching, "dolphin/OdemuExi2/DebuggerTransport.c", extra_cflags=["-inline deferred"]),
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
            Object(Matching, "dolphin/base/ppc_registers.s", add_to_all=False),
            Object(Matching, "dolphin/base/PPCArch.c"),
            Object(Matching, "dolphin/base/ppc_fp_mode.s", add_to_all=False),
            Object(Equivalent, "dolphin/base/ppc_register_helpers.s", add_to_all=False),
        ],
    ),
    DolphinLib(
        "db",
        [
            Object(Matching, "dolphin/db/db_exception.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/db/db_exception_entry.s", add_to_all=False),
            Object(Matching, "dolphin/db/db.c", cflags=cflags_sdk_2004),
        ],
    ),
    DolphinLib(
        "os",
        [
            # The retail units interleave the C code and hardware exception entry points.
            Object(Equivalent, "dolphin/os/OS.c", cflags=cflags_sdk_2004),
            Object(Equivalent, "dolphin/os/OSAlarm.c", cflags=cflags_sdk_2004),
            Object(Equivalent, "dolphin/os/os_boot_fpu.s", add_to_all=False),
            Object(
                Equivalent,
                "dolphin/os/os_exception_vectors.s",
                add_to_all=False,
                extra_as_implicit=["include/sdk/private/os/exception_context.inc"],
            ),
            Object(Matching, "dolphin/os/OSAlloc.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/os/OSArena.c"),
            # The C code preserves DSP mailbox access order with a smaller stack frame.
            Object(Equivalent, "dolphin/os/OSAudioSystem.c"),
            Object(Matching, "dolphin/os/os_cache_lines.s", add_to_all=False),
            Object(
                Equivalent,
                "dolphin/os/os_cache_helpers.s",
                source="dolphin/os/os_cache_lines.s",
                extra_asflags=["--defsym OS_CACHE_HELPERS_ONLY=1"],
                add_to_all=False,
            ),
            Object(Equivalent, "dolphin/os/OSLockedCache.c"),
            Object(Equivalent, "dolphin/os/os_locked_cache.s", add_to_all=False),
            Object(Equivalent, "dolphin/os/OSCache.c"),
            Object(Equivalent, "dolphin/os/OSContext.c"),
            Object(
                Equivalent,
                "dolphin/os/os_context_switch.s",
                add_to_all=False,
                extra_as_implicit=["include/sdk/private/os/exception_context.inc"],
            ),
            Object(
                Equivalent,
                "dolphin/os/os_fpu_context.s",
                add_to_all=False,
                extra_as_implicit=["include/sdk/private/os/exception_context.inc"],
            ),
            Object(Matching, "dolphin/os/OSError.c"),
            Object(Equivalent, "dolphin/os/OSExec.c", cflags=cflags_sdk_2004),
            Object(Equivalent, "dolphin/os/os_exec_run.s", add_to_all=False),
            Object(Matching, "dolphin/os/OSFatal.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/os/OSFont.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/os/os_interrupt_mask.s", add_to_all=False),
            Object(Equivalent, "dolphin/os/OSInterrupt.c"),
            Object(
                Matching,
                "dolphin/os/os_interrupt_entry.s",
                add_to_all=False,
                extra_as_implicit=["include/sdk/private/os/exception_context.inc"],
            ),
            Object(Matching, "dolphin/os/OSLink.c", cflags=cflags_sdk_2004, padding="small_data+data"),
            Object(Matching, "dolphin/os/OSLinkNotify.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/os/OSMessage.c"),
            Object(Equivalent, "dolphin/os/OSMemory.c"),
            Object(Matching, "dolphin/os/os_memory_mapping.s", add_to_all=False),
            Object(NonMatching, "dolphin/os/OSMemory_protection.c"),
            Object(Matching, "dolphin/os/OSMutex.c"),
            Object(Matching, "dolphin/os/OSReboot.c", cflags=cflags_sdk_2004),
            Object(Equivalent, "dolphin/os/OSReset.c", cflags=cflags_sdk_2004),
            Object(Equivalent, "dolphin/os/os_hot_reset.s", add_to_all=False),
            Object(Matching, "dolphin/os/OSResetSW.c"),
            Object(Matching, "dolphin/os/OSRtc.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/os/OSStopwatch.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/os/os_syscall_vector.s", add_to_all=False),
            Object(Equivalent, "dolphin/os/OSSync.c"),
            Object(Equivalent, "dolphin/os/OSThread.c"),
            Object(Matching, "dolphin/os/os_timebase.s", add_to_all=False),
            Object(Equivalent, "dolphin/os/OSTime.c", cflags=cflags_sdk_2004),
            Object(Equivalent, "dolphin/os/os_timebase_write.s", add_to_all=False),
            Object(Equivalent, "dolphin/os/os_fast_cast.s", add_to_all=False),
            Object(Equivalent, "dolphin/os/os_boot_hardware.s", add_to_all=False),
            Object(Equivalent, "dolphin/os/__ppc_eabi_init.cpp"),
            Object(Equivalent, "dolphin/os/__start.c"),
            Object(Equivalent, "dolphin/os/os_start.s", add_to_all=False),
        ],
    ),
    DolphinLib(
        "mtx",
        [
            # The C rotation wrappers separate three runs of Gekko paired-single code.
            Object(
                Matching,
                "dolphin/mtx/mtx_paired.s",
                add_to_all=False,
                extra_asflags=["--defsym MTX_PAIRED_SECTION=0"],
            ),
            Object(Equivalent, "dolphin/mtx/mtx.c"),
            Object(
                Matching,
                "dolphin/mtx/mtx_paired_rotation.s",
                source="dolphin/mtx/mtx_paired.s",
                add_to_all=False,
                extra_asflags=["--defsym MTX_PAIRED_SECTION=1"],
            ),
            Object(Equivalent, "dolphin/mtx/mtx_rot_axis.c"),
            Object(
                Matching,
                "dolphin/mtx/mtx_paired_affine.s",
                source="dolphin/mtx/mtx_paired.s",
                add_to_all=False,
                extra_asflags=["--defsym MTX_PAIRED_SECTION=2"],
            ),
            Object(Equivalent, "dolphin/mtx/mtx_view.c"),
            Object(Equivalent, "dolphin/mtx/mtxvec.c"),
            Object(Matching, "dolphin/mtx/mtx44.c"),
            # Two C routines sit between the three runs of paired-single code.
            Object(
                Matching,
                "dolphin/mtx/vec_paired.s",
                add_to_all=False,
                extra_asflags=["--defsym VEC_PAIRED_SECTION=0"],
            ),
            Object(Matching, "dolphin/mtx/vec.c"),
            Object(
                Matching,
                "dolphin/mtx/vec_paired_cross.s",
                source="dolphin/mtx/vec_paired.s",
                add_to_all=False,
                extra_asflags=["--defsym VEC_PAIRED_SECTION=1"],
            ),
            Object(Equivalent, "dolphin/mtx/vec_reflect.c"),
            Object(
                Matching,
                "dolphin/mtx/vec_paired_distance.s",
                source="dolphin/mtx/vec_paired.s",
                add_to_all=False,
                extra_asflags=["--defsym VEC_PAIRED_SECTION=2"],
            ),
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
            Object(Matching, "dolphin/dvd/dvderrorcode.c"),
            Object(Matching, "dolphin/dvd/dvdidutils.c"),
            Object(Matching, "dolphin/dvd/dvdFatal.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/dvd/fstload.c", cflags=cflags_sdk_2004, extra_cflags=["-char signed"]),
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
            Object(Equivalent, "dolphin/ai/ai.c"),
            Object(Equivalent, "dolphin/ai/ai_callback_stack.s", add_to_all=False),
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
            Object(Equivalent, "dolphin/ax/AXVPB.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/ax/AXComp.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/ax/DSPCode.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/ax/AXProf.c", cflags=cflags_sdk_2004),
        ],
    ),
    DolphinLib(
        "axfx",
        [
            Object(Equivalent, "dolphin/axfx/reverb_hi.c", cflags=cflags_sdk_2004),
            Object(Equivalent, "dolphin/axfx/reverb_std.c", cflags=cflags_sdk_2004),
            Object(Equivalent, "dolphin/axfx/chorus.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/axfx/delay.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/axfx/axfx.c", cflags=cflags_sdk_2004),
        ],
    ),
    DolphinLib(
        "dsp",
        [
            Object(Matching, "dolphin/dsp/dsp.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/dsp/dsp_debug.c"),
            Object(Equivalent, "dolphin/dsp/dsp_task.c", cflags=cflags_sdk_2004),
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
            Object(Equivalent, "dolphin/gx/GXInit.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/gx/GXFifo.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/gx/GXAttr.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/gx/GXMisc.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/gx/GXGeometry.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/gx/GXFrameBuf.c", cflags=cflags_sdk_2004),
            Object(Equivalent, "dolphin/gx/GXLight.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/gx/GXTexture.c"),
            Object(Matching, "dolphin/gx/GXTextureState.c"),
            Object(Matching, "dolphin/gx/GXBump.c"),
            Object(Equivalent, "dolphin/gx/GXTev.c", cflags=cflags_sdk_2004),
            Object(Equivalent, "dolphin/gx/GXPixel.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/gx/GXDraw.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/gx/GXDisplayList.c"),
            Object(Equivalent, "dolphin/gx/GXTransform.c", cflags=cflags_sdk_2004),
            Object(Matching, "dolphin/gx/GXPerf.c"),
        ],
    ),
    DolphinLib(
        "arc",
        [
            Object(Equivalent, "dolphin/arc/arc.c"),
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
            Object(Equivalent, "game/ut.cpp", cflags=cflags_hsd_game, extra_cflags=["-inline auto,deferred"]),
            Object(Equivalent, "game/mem.cpp"),
            Object(Equivalent, "game/ccamera.cpp", cflags=cflags_hsd_game, extra_cflags=["-inline auto,deferred"]),
            Object(Matching, "game/clight.cpp", cflags=cflags_hsd_game, extra_cflags=["-inline auto,deferred"]),
            Object(Matching, "game/cshadow.cpp", cflags=cflags_hsd_game, extra_cflags=["-inline auto,deferred"]),
            Object(Equivalent, "game/cgs.cpp", cflags=cflags_hsd_game, extra_cflags=["-inline auto,deferred"]),
            Object(Equivalent, "game/cgsprimitive.cpp", cflags=cflags_hsd_game, extra_cflags=["-inline auto,deferred"]),
            Object(Equivalent, "game/cgsinit.cpp", cflags=cflags_hsd_game, extra_cflags=["-inline auto,deferred"]),
            Object(Equivalent, "game/cgsstate.cpp", cflags=cflags_hsd_game, extra_cflags=["-inline auto,deferred"]),
            Object(Equivalent, "game/cprimitive.cpp", extra_cflags=["-inline auto,deferred"]),
            Object(Matching, "game/cpad.cpp", cflags=cflags_hsd_game, extra_cflags=["-inline auto,deferred"]),
            Object(Equivalent, "game/cfont.cpp"),
            Object(Equivalent, "game/cfile.cpp"),
            Object(NonMatching, "game/cfilearchivedtor.cpp", source="game/cfile.cpp"),
            Object(Equivalent, "game/ctpl.cpp"),
            Object(NonMatching, "game/ctpldtor.cpp", source="game/ctpl.cpp"),
            Object(NonMatching, "game/cobj.cpp", cflags=cflags_hsd_game, extra_cflags=["-inline auto,deferred"]),
            Object(Equivalent, "game/ctrigon.cpp", cflags=cflags_hsd_game, extra_cflags=["-inline auto,deferred"]),
            Object(Matching, "game/ctoon.cpp", cflags=cflags_hsd_game, extra_cflags=["-inline auto,deferred"]),
            Object(Equivalent, "game/fbeffect.cpp", extra_cflags=["-inline auto,deferred"]),
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
            Object(Equivalent, "sysdolphin/baselib/shadow.c", extra_cflags=["-inline auto,deferred"]),
            Object(Equivalent, "sysdolphin/baselib/pobj.c", extra_cflags=["-inline auto,deferred"]),
            Object(Equivalent, "sysdolphin/baselib/tobj.c", extra_cflags=["-inline auto,deferred"], padding="small_data+rodata"),
            Object(Equivalent, "sysdolphin/baselib/psdisp.c", extra_cflags=["-inline auto,deferred"]),
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
            Object(Equivalent, "game/xobjhboxupdate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/plugcordsimulation.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjchrsuitjoints.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjspiderbossevent.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjchrdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjactorjointupdate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/plugcordpoints.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjspiderbossplacement.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/plugcordpush.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjchrattachmentchange.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjactorevent.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjchrdisplay.cpp", cflags=with_hsd_headers(cflags_game)),
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
            Object(Equivalent, "sysdolphin/baselib/tev.c"),
            Object(Equivalent, "sysdolphin/baselib/mobj.c"),
            Object(Matching, "sysdolphin/baselib/aobj.c", padding="small_data+data"),
            Object(Equivalent, "sysdolphin/baselib/lobj.c"),
            Object(Equivalent, "sysdolphin/baselib/cobj.c"),
            Object(Matching, "sysdolphin/baselib/fobj.c", padding="small_data+data"),
            Object(Equivalent, "sysdolphin/baselib/jobj.c"),
            Object(Matching, "sysdolphin/baselib/displayfunc.c", padding="small_data+data"),
            Object(Matching, "sysdolphin/baselib/initialize.c"),
            Object(Equivalent, "sysdolphin/baselib/video.c", padding="small_data+data"),
            Object(Matching, "sysdolphin/baselib/controller.c"),
            Object(Matching, "sysdolphin/baselib/rumble.c"),
            Object(Matching, "sysdolphin/baselib/spline.c"),
            Object(Matching, "sysdolphin/baselib/mtx.c"),
            Object(Matching, "sysdolphin/baselib/util.c"),
            Object(Matching, "sysdolphin/baselib/objalloc.c"),
            Object(Equivalent, "sysdolphin/baselib/robj.c"),
            Object(Matching, "sysdolphin/baselib/id.c"),
            Object(Matching, "sysdolphin/baselib/wobj.c"),
            Object(Matching, "sysdolphin/baselib/fog.c"),
            Object(Matching, "sysdolphin/baselib/perf.c"),
            Object(Matching, "sysdolphin/baselib/list.c"),
            Object(Matching, "sysdolphin/baselib/object.c"),
            Object(Equivalent, "sysdolphin/baselib/quatlib.c"),
            Object(Matching, "sysdolphin/baselib/memory.c", padding="small_data+data"),
            Object(Matching, "sysdolphin/baselib/archive.c"),
            Object(Matching, "sysdolphin/baselib/random.c"),
            Object(Equivalent, "sysdolphin/baselib/bytecode.c"),
            Object(Equivalent, "sysdolphin/baselib/class.c"),
            Object(Matching, "sysdolphin/baselib/hash.c"),
            # TExp2TevDesc still differs in register allocation.
            Object(Equivalent, "sysdolphin/baselib/texp.c"),
            Object(Matching, "sysdolphin/baselib/texpdag.c"),
            Object(Equivalent, "sysdolphin/baselib/texpopt.c"),
            # Keep the hardware snapshot in the extracted object for normal builds.
            Object(
                Matching,
                "sysdolphin/baselib/debug_context.s",
                add_to_all=False,
                extra_as_implicit=["include/sdk/private/os/exception_context.inc"],
            ),
            Object(Matching, "sysdolphin/baselib/debug.c", padding="small_data+data"),
            # The source particle and generator units share a command-bank lookup.
            Object(Equivalent, "sysdolphin/baselib/particle.c"),
            Object(Equivalent, "sysdolphin/baselib/psinterpret.c", padding="small_data+data"),
            Object(Equivalent, "sysdolphin/baselib/generator.c", padding="small_data+data"),
            Object(Matching, "sysdolphin/baselib/psdisptev.c"),
            Object(Matching, "sysdolphin/baselib/psappsrt.c"),
            Object(Matching, "sysdolphin/baselib/pslist.c"),
            Object(Equivalent, "sysdolphin/baselib/axdriver.c"),
            Object(Equivalent, "sysdolphin/baselib/devcom.c"),
            Object(Matching, "sysdolphin/baselib/dvdsche.c"),
            Object(Equivalent, "sysdolphin/baselib/synth_bank.c"),
            Object(NonMatching, "sysdolphin/baselib/synth.c"),
            Object(Equivalent, "sysdolphin/baselib/synth_mix.c"),
            Object(Equivalent, "sysdolphin/baselib/synth_hako.c"),
            Object(NonMatching, "sysdolphin/baselib/synth_stream.c"),
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
            Object(Equivalent, "dolphin/thp/THPDec.c"),
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
            Object(Equivalent, "rel/shun/shun.cpp"),
            Object(Equivalent, "rel/fafa.cpp"),
            Object(Equivalent, "rel/stage01/hamburg.cpp"),
            Object(Equivalent, "rel/ana.cpp", extra_cflags=["-inline auto,deferred"]),
            Object(Equivalent, "rel/stage02/fighter.cpp"),
            Object(Equivalent, "rel/stage02/rouka.cpp", extra_cflags=["-inline auto,deferred"]),
            Object(Equivalent, "rel/stage02/car.cpp", extra_cflags=["-inline auto,deferred"]),
            Object(Equivalent, "rel/stage03/decapanel.cpp"),
            Object(Matching, "rel/stage04/jenny.cpp"),
            Object(NonMatching, "rel/trash.cpp"),
            Object(Equivalent, "rel/stage06/parent.cpp"),
            Object(Equivalent, "rel/stage07/living.cpp"),
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


def link_order_callback(module_id: int, objects: List[str]) -> List[str]:
    if config.non_matching and module_id == 0:
        # Source-built constructors and destructors emit these ABI symbols.
        # CGs owns the primitive members and emits their weak destructors.
        # The source-built OSMemory.c also defines the protection initializer.
        extracted_only = {
            "game/xobjattachment_rtti.cpp",
            "game/logo_rtti.cpp",
            "game/oxmenu_rtti.cpp",
            "game/title_title_vtable.cpp",
            "game/title_cursor_vtable.cpp",
            "game/title_xobjdemo_vtable.cpp",
            "game/title_xobj_vtable.cpp",
            "game/title_cxobj_vtables.cpp",
            "game/menuex_vtable.cpp",
            "game/menucursor_vtable.cpp",
            "game/cmsg_vtable.cpp",
            "game/drsmenu_vtable.cpp",
            "game/atcmenu_vtables.cpp",
            "game/cscrnwipecircle_rtti.cpp",
            "game/cscrnwipe_rtti.cpp",
            "game/cfboutline_rtti.cpp",
            "game/cthread_vtable.cpp",
            "game/arprecache_vtable.cpp",
            "game/arcacheinfo_vtable.cpp",
            "game/cmovie_vtable.cpp",
            "game/ctrigon_vtable.cpp",
            "game/xobjactor3_vtable.cpp",
            "game/cfbtexeff_vtable.cpp",
            "game/fbsubwin_vtable.cpp",
            "game/fbmirror_vtable.cpp",
            "game/cinterpolationeaseout_dtors.cpp",
            "game/cinterpolationeaseout_rtti.cpp",
            "game/cinterpolationnone_vtable.cpp",
            "game/cvectornoneinterpolation_vtable.cpp",
            "game/cinterfacevectorinterpolation_rtti.cpp",
            "game/cinterfacecameraaction_vtables.cpp",
            "game/cinterpolationlinear_vtable.cpp",
            "game/playerattachmentpaths.cpp",
            "game/cvectoreaseoutdivideinterpolation_vtable.cpp",
            "game/cinterpolationeaseoutdivide_vtable.cpp",
            "game/cjanim_vtable.cpp",
            "game/csound_vtable.cpp",
            "game/xsound_vtable.cpp",
            "game/cjointrot_vtable.cpp",
            "game/params_vtable.cpp",
            "game/xpad_vtable.cpp",
            "game/memcard_vtable.cpp",
            "game/memcardscan_format.cpp",
            "game/memcardscan_buffer.cpp",
            "game/memcardpreview_buffers.cpp",
            "game/cgenerator_vtable.cpp",
            "game/csounddtor.cpp",
            "game/gameccameraactions_tail.cpp",
            "game/cmotioncontrolcamera_vtable.cpp",
            "game/ccameraactions_vtable.cpp",
            "game/ccameraactionarea_vtable.cpp",
            "game/ccameraactionmap_vtable.cpp",
            "game/ccameraactionreset_vtable.cpp",
            "game/gamecgenerator.cpp",
            "game/cprimitivedtor.cpp",
            "game/cspritedtor.cpp",
            "game/ctiledtor.cpp",
            "game/cgschannelctor.cpp",
            "game/gamecgschan.cpp",
            "game/cfilearchivedtor.cpp",
            "game/cfilector.cpp",
            "game/attrinfodtor.cpp",
            "game/ccolordtor.cpp",
            "game/cgsscreendtor.cpp",
            "game/cfiletext_rtti.cpp",
            "game/cjarchive_rtti.cpp",
            "game/xobjctor.cpp",
            "game/cevtdtor.cpp",
            "game/cinterfacevectorinterpolationdtor.cpp",
            "game/cpaddtor.cpp",
            "game/cpadexdtor.cpp",
            "game/cstatus64dtor.cpp",
            "game/cstatusdtor.cpp",
            "game/cvecctor.cpp",
            "game/cxobjintpctor.cpp",
            "game/clightctor.cpp",
            "game/cptcldtor.cpp",
            "game/cshadowdtor.cpp",
            "game/ctpldtor.cpp",
            "game/gameccamera.cpp",
            "game/gamecfile.cpp",
            "game/gamecobjarchive.cpp",
            "game/gamecoordinfo.cpp",
            "game/gamecstopwatch.cpp",
            "dolphin/os/OSMemory_protection.c",
        }
        objects = [obj for obj in objects if obj not in extracted_only]
        # These SDK assembly units have no separate retail objects.
        return objects + [
            "dolphin/base/ppc_register_helpers.s",
            "dolphin/ai/ai_callback_stack.s",
            "dolphin/os/os_boot_fpu.s",
            "dolphin/os/os_exception_vectors.s",
            "dolphin/os/os_cache_helpers.s",
            "dolphin/os/os_locked_cache.s",
            "dolphin/os/os_context_switch.s",
            "dolphin/os/os_fpu_context.s",
            "dolphin/os/os_exec_run.s",
            "dolphin/os/os_start.s",
            "dolphin/os/os_hot_reset.s",
            "dolphin/os/os_timebase_write.s",
            "dolphin/os/os_fast_cast.s",
        ]
    # Don't modify the link order for matching builds
    if not config.non_matching:
        return objects
    if module_id == 0:  # DOL
        result = []
        for obj in objects:
            if obj == "revolution/OS/__ppc_eabi_init.cpp":
                result.append("revolution/OS/os_ppc_eabi_init.s")
            if obj == "revolution/BASE/PPCArch.c":
                result.append("revolution/BASE/ppc_registers.s")
            result.append(obj)
            if obj == "revolution/BASE/PPCArch.c":
                result.append("revolution/BASE/ppc_fp_mode.s")
                result.append("revolution/BASE/ppc_hid4.s")
            if obj == "revolution/OS/OS.c":
                result.append("revolution/OS/os_boot_fpu.s")
            if obj == "revolution/OS/OSCache.c":
                result.append("revolution/OS/os_cache_lines.s")
                result.append("revolution/OS/os_locked_cache.s")
            if obj == "revolution/OS/OSContext.c":
                result.append("revolution/OS/os_context_switch.s")
                result.append("revolution/OS/os_fpu_context.s")
            if obj == "revolution/OS/OSInterrupt.c":
                result.append("revolution/OS/os_interrupt_mask.s")
                result.append("revolution/OS/os_interrupt_entry.s")
            if obj == "revolution/OS/OSMemory.c":
                result.append("revolution/OS/os_memory_mapping.s")
            if obj == "revolution/OS/OSExec.c":
                result.append("revolution/OS/os_exec_run.s")
            if obj == "dolphin/ai/ai.c":
                result.append("dolphin/ai/ai_callback_stack.s")
            if obj == "revolution/AI/ai.c":
                result.append("revolution/AI/ai_callback_stack.s")
            if obj == "revolution/MTX/mtx44.c":
                result.append("revolution/MTX/mtx44_paired.s")
            if obj == "revolution/GX/GXLight.c":
                result.append("revolution/GX/gx_light_load.s")
            if obj == "revolution/GX/GXTransform.c":
                result.append("revolution/GX/gx_transform_ps.s")
            if obj == "rvl_runtime/__init_cpp_exceptions.cpp":
                result.append("rvl_runtime/init_cpp_exceptions.s")
            if obj == "rvl_trk/targimpl.c":
                result.append("TRK_MINNOW_DOLPHIN/trk_target_context.s")
            if obj == "rvl_trk/dolphin_trk.c":
                result.append("TRK_MINNOW_DOLPHIN/trk_boot.s")
            if obj == "rvl_trk/dolphin_trk_glue.c":
                result.append("TRK_MINNOW_DOLPHIN/trk_context_load.s")
            if obj == "revolution/DB/db.c":
                result.append("revolution/DB/db_exception_entry.s")
            if obj == "revolution/OS/OSAlarm.c":
                result.append("revolution/OS/os_exception_vectors.s")
        return result
    return objects


config.link_order_callback = link_order_callback


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
config.progress_report_args = [
    "--config functionRelocDiffs=data_value",
]


def append_treemap_step(config: ProjectConfig) -> None:
    # Reports are added after custom project steps.
    report = (config.out_path() / "report.json").as_posix()
    page = (config.build_dir / "treemap.html").as_posix()
    with open("build.ninja", "a", encoding="utf-8") as f:
        f.write(
            f"\n{CONFIGURE_ARGS_PREFIX}{json.dumps(sys.argv[1:])}\n"
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
