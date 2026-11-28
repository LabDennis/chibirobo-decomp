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
    "R24J01",  # Wii, Japan rev 0
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
wii = config.version == "R24J01"

# Apply arguments
config.build_dir = args.build_dir
config.dtk_path = args.dtk
config.objdiff_path = args.objdiff
config.binutils_path = args.binutils
config.compilers_path = args.compilers
# The US and Japanese GameCube linkers used ASCII '0' for padding.
config.fill_padding = config.version in ("GGTE01", "GGTJ01_01")
config.generate_map = args.map or not wii
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
    config.ldflags.append("-gdwarf-2" if wii else "-g")
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

config.linker_version = "Wii/1.3" if wii else "GC/2.6"


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
            Object(NonMatching, "game/ccameraactionarea_vtable.cpp"),
            Object(NonMatching, "game/ccameraactionmap_vtable.cpp"),
            Object(NonMatching, "game/ccameraactionreset_vtable.cpp"),
            Object(NonMatching, "game/cjointrot_vtable.cpp"),
            Object(NonMatching, "game/params_vtable.cpp"),
            Object(NonMatching, "game/xpad_vtable.cpp"),
            Object(NonMatching, "game/drsmenu_vtable.cpp"),
            Object(NonMatching, "game/atcmenu_vtables.cpp"),
            # CGs generates these member constructors and destructors.
            Object(NonMatching, "game/fbmirrorex.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/sound.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/screenwipe.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/gxb.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/titlescreen.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/stg.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/infoscrn.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/stage_data.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/stage_defaults.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/stage_tables.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/obja.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/obja_map.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/obja_chr.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/objb_act.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/objb_attach.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/objb_plug.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/objb_sound.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/objb_leticker.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/objb_army.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjspider.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjspidersmall.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjspiderbossdraw.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjspiderbossreset.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjspiderbossdata.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/memcardrestore.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(MatchingFor(JP, PAL) or Equivalent, "game/memcardcapture.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/memcardbanner.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactor3.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactortoy.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactorjenny.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactormama.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactorpapa.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactortao.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactordeca.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/memcardinit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/memcardcamera.cpp", cflags=with_hsd_headers(cflags_game)),
            # The generated vtable still needs a split that preserves the class's section order.
            Object(NonMatching, "game/xobjspiderbossdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjcameradtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjlightdtor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjplugcord.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjspiderbossstop.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/memcardheader.cpp", cflags=cflags_game),
            Object(Equivalent, "game/memcardload.cpp", cflags=cflags_game),
            Object(Equivalent, "game/memcardscan.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/memcardscan_format.cpp"),
            Object(NonMatching, "game/memcardscan_buffer.cpp"),
            Object(NonMatching, "game/memcardpreview_buffers.cpp"),
            Object(Equivalent, "game/xobjspiderbossstate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjspiderbossupdate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjspiderbossinit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjspiderbossgetup.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/memcardpos.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(MatchingFor(JP, PAL) or Equivalent, "game/msgdataformat.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/memcardreadstate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/memcardwritestate.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjspiderbossattack.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/memcardcheck.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjfree.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/memcardformat.cpp", cflags=cflags_game),
            Object(Equivalent, "game/memcardread.cpp", cflags=cflags_game),
            Object(Equivalent, "game/xobjspiderbosshit.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/memcardscanjob.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjspiderbossse.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjspiderbossconstants.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjspiderbossstates34.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/xobjspiderbossload.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/memcardfile.cpp", cflags=cflags_game),
            Object(NonMatching, "game/params.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/inventory.cpp"),
            Object(Equivalent, "game/xobjcamera.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjlight.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/xobjactor.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Matching, "game/memcardjob.cpp"),
            Object(Equivalent, "game/cinterpolationeaseout.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/cinterpolationeaseout_rtti.cpp"),
            Object(NonMatching, "game/ccameraactionareaex.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(NonMatching, "game/evt.cpp", cflags=with_hsd_headers(cflags_game)),
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
            Object(Equivalent, "game/cprimitive.cpp", extra_cflags=["-inline auto,deferred"]),
            Object(Matching, "game/cpad.cpp", cflags=cflags_hsd_game, extra_cflags=["-inline auto,deferred"]),
            Object(Equivalent, "game/cfont.cpp"),
            Object(Equivalent, "game/cfile.cpp"),
            Object(Equivalent, "game/ctpl.cpp"),
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
            Object(Equivalent, "game/xobjspiderbossevent.cpp", cflags=with_hsd_headers(cflags_game)),
            Object(Equivalent, "game/xobjspiderbossplacement.cpp", cflags=with_hsd_headers(cflags_game)),
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


if wii:
    wii_cflags = [
        "-nodefaults", "-proc gekko", "-align powerpc", "-enum int", "-fp hardware",
        "-O4,p", "-func_align 4", "-inline auto", "-use_lmw_stmw on", "-fp_contract on",
        "-nosyspath", "-gccinc", "-i include", "-i include/stl", f"-i build/{config.version}/include",
        "-DVERSION_R24J01",
        f"-DDTK_CONFIG_NONMATCHING={config.non_matching:d}",
    ]
    if args.warn:
        wii_cflags.append(f"-W {args.warn}")
    wii_cflags.append("-DDEBUG=1" if args.debug else "-DNDEBUG=1")
    # the Wii's GX sources take the SDK's public headers from include/sdk
    # (ahead of include/, whose Dolphin/ directory answers to <dolphin/...> as well)
    wii_gx_cflags = [
        "-i include/sdk", "-i include/sdk/dolphin", "-i include/sdk/msl/c",
        *wii_cflags, "-Cpp_exceptions off", "-fp_contract off", "-common off", "-ipa file", "-use_lmw_stmw off",
    ]
    # Several Wii SDK units use the SDK's dolphin-style public and private headers.
    # Only those include paths are unit-specific; with them the SDK-wide code generation
    # flags emit byte-identical allocatable sections in the audited units, so the older
    # wholesale cflags_sdk_2004 profile is not retail evidence there.
    wii_dolphin_header_base_cflags = [
        "-i include/sdk", "-i include/sdk/dolphin", "-i include/sdk/msl/c", "-ir include/sdk/private",
        *wii_cflags, "-Cpp_exceptions off", "-fp_contract off", "-common off",
    ]
    wii_dolphin_header_cflags = [*wii_dolphin_header_base_cflags, "-ipa file", "-use_lmw_stmw off"]
    wii_dolphin_header_noipa_cflags = [*wii_dolphin_header_base_cflags, "-use_lmw_stmw off"]
    wii_dolphin_header_lmw_cflags = [*wii_dolphin_header_base_cflags, "-ipa file", "-use_lmw_stmw on"]
    # the Wii's MSL: its own headers (include/rvl_msl/msl) ahead of the shared ones
    wii_msl_cflags = [
        "-i include/rvl_msl/msl", *wii_cflags,
        "-Cpp_exceptions on", "-str reuse,pool,readonly", "-fp_contract off", "-inline on", "-ipa file",
    ]
    config.libs = [
        {
            # -i include/rvl_msl: game/wii/csound.h holds the nw4r sound classes, whose headers need the Wii MSL
            "lib": "game",
            "mw_version": "Wii/1.3",
            "cflags": [*wii_cflags, "-i include/rvl_msl", "-Cpp_exceptions off", "-RTTI on", "-str reuse,readonly"],
            "progress_category": "game",
            "objects": [
                # Two groups of units. Those with an mw_version are built with GC/3.0a5 and -ipa file:
                # - compiler: CModule::Unload, OctaMenu::UpdatePointedItem and CPadAct::Init/Update score
                #   strictly higher under GC/3.0 than under Wii/1.3, which also emits @NNNN sdata2 objects
                #   where retail has the same literals as named objects. GC/3.0a3 to 3.0a5 give the same
                #   code in every unit of the group; 3.0a5.2 differs from retail in homemenu.cpp
                #   (0x800AB3E8) and init.cpp (the light vectors' registers) and nowhere helps.
                # - -ipa file: homemenu.cpp and init.cpp need it (see there). On the other units of the
                #   group it raises six functions (AdvanceStageLoad and RunWiiSaveSlotDelete to 100 %)
                #   and lowers one by 0.01 (CPadAct::Update, 94.76 to 94.75).
                # The units without an mw_version keep the library's Wii/1.3: nandfile.cpp matches only
                # so (its twelve functions fall to about 80 % under GC/3.0a5 with -ipa file).
                # the Wii file defines Load, Unload, Print in that order (the GameCube one the reverse)
                Object(Matching, "game/cmodule.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on"]),
                Object(Equivalent, "game/ringmenupointer.cpp", source="game/wii/ringmenupointer.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # the THP player sits apart from the game's files (0x802C27B4 on) and takes the compiler
                # without -ipa file: with it MixAudio falls to 99.88 %.
                # -inline noauto: retail THPSimple keeps its small static helper functions out-of-line
                # rather than auto-inlining them (matching the Revolution/Dolphin THPPlayer demo build).
                Object(Equivalent, "game/THPDraw.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on"]),
                Object(Matching, "game/THPSimple.cpp", source="game/wii/THPSimple.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-inline noauto"]),
                # one retail file on Wii (0x80092308-0x8009453C): stage_data + globalresources in forward
                # source order, sharing gStageResources/gGlobalScriptResources, @stringBase0, .data and .sdata2
                Object(Equivalent, "game/wii/stage_data.cpp", mw_version="GC/3.0a5",
                       extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                Object(Equivalent, "game/wii/atcmenuselection.cpp"),
                # the whole file, 0x800AAD1C-0x800AC07C. GC/3.0a3 to 3.0a5 reproduce it; 3.0a5.2 puts the
                # li r0, 0 at 0x800AB3E8 behind the branch that follows it in retail, Wii/1.x turns the
                # load step's switch into a compare chain. -ipa file: retail has the eleven hbm/ paths
                # as objects of their own in front of @stringBase0, without it they join that pool
                Object(Matching, "game/wii/homemenu.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                Object(NonMatching, "game/wii/title.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                Object(NonMatching, "game/wii/menu.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # the hud unit 0x800870E8-0x80092308, ported from the GameCube's infoscrn.cpp:
                # GetStatusFlags replaces mStatus.mFlags reads, the timer runs off gRefreshRate.
                # Its file static counters bring the weak InfoPoint dtor and the static
                # initializer at the end (retail addresses them off the unit's .bss start)
                Object(NonMatching, "game/wii/infoscrn.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # no auto-inlining of the small file functions: under -inline auto the
                # compiler inlines OpenPath/GetResult/ReadAsync into their callers where
                # retail keeps them as calls
                Object(Matching, "game/wii/cfile.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # the Wii texture/tpl + sound unit 0x802B4DAC-0x802B6094 (ctpl.cpp by its GC
                # file): CTexObj's quad draws, CTpl's TPL bind and the game's nw4r sound
                # setup -- loader threads, archive open/group load, track fades, 3D actor init
                Object(NonMatching, "game/wii/ctpl.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # the Wii-only resource file unit 0x802B6094-0x802B6560 (cresfile.cpp by its
                # OSPanic file): ResourceFile's refcounted CFile, the CFileRequest state
                # machine and CResFile's brres accessor over the nw4r walkers
                Object(NonMatching, "game/wii/cresfile.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # the model objects 0x802B6560-0x802BA4F4 (cobj.cpp by its assert file) and the particle
                # effects 0x802BD3B4-0x802BF9F8 (ceffect.cpp): only what does not go through nw4r::g3d
                # or nw4r::ef is written, the rest waits on those libraries' headers
                Object(NonMatching, "game/wii/cobj.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                Object(NonMatching, "game/wii/ceffect.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                # the frame buffer effects 0x802BF9F8-0x802C19B8 (fbeffect.cpp by its string pool):
                # FbBase, FbBlur and FbEffect, the GameCube's CFbBlur and CFbOutline on a common base
                Object(NonMatching, "game/wii/fbeffect.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # the RFNU/RFNT font file loader + glyph cache between the console font paths;
                # the small setup helpers stay calls in retail, so no auto-inlining here either
                Object(Matching, "game/wii/cfont.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # init.cpp (the name is in its OSPanic call): InitTwoLightSet and InitFourLightSet
                # copy their vectors through the registers retail uses under GC/3.0a3.2 to 3.0a5
                # and through others under 3.0a5.2, and every Wii build is far off. -ipa file: the
                # weak functions a function leaves behind (CCamera::CalcAngle, CalcDistance;
                # CLight::SetPosition, vf30) follow it in the order of their calls, as in retail
                # 0x8003607C and 0x80036254; without the flag they come out in the reverse order
                Object(NonMatching, "game/wii/init.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # the whole file, linked. No auto-inlining of the small pad functions: under
                # -inline auto the compiler inlines Collect*/Init into their callers where retail
                # keeps them as calls; IsGameCubePadActive is inline-declared and still inlined
                # into Update (no bl, retail 0x802B2528), with its standalone copy at its own
                # address (retail 0x802B1D84)
                Object(Matching, "game/wii/cpad.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # cgs.cpp from the camera on, 0x802A80FC-0x802AEA44; it includes cgs_scrn.cpp and
                # cgs_chan.cpp, whose asserts share its string pool. No auto-inlining:
                # retail calls DrawGsTriangleVertices from DrawGsVertexQuad and MeasureStrokeTextWidth
                # from DrawGsStrokeText, both of which -inline auto folds into their one caller
                # -i include/rvl_msl: <cmath>'s std::sinf/cosf, which the Wii MSL provides
                Object(NonMatching, "game/wii/cgs.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                # NW4R scene adapter after cgs.cpp's static initialiser, named by its pointer assert
                Object(NonMatching, "game/wii/cgs_nw.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                # the message system between evt.cpp's destructors and cpadact.cpp; the
                # GameCube file's VERSION_JP branches are the reconstruction base
                Object(NonMatching, "game/wii/msg.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                # the Wii's shared helper unit 0x802A5358-0x802A7478: Shift-JIS, HSD matrix
                # and collision helpers from the GameCube's ut.cpp + mtx.c; the CVec
                # operators and PSVEC calls stay calls, so no auto-inlining
                # -i include/rvl_msl: <cmath>'s std::sinf/cosf, which the Wii MSL provides
                Object(NonMatching, "game/wii/ut.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                # the Wii memory manager 0x802A7478-0x802A7FB4 (mem.cpp by its OSPanic file):
                # the GameCube table allocator over a MEM2 arena plus InitMem1/InitMem2's two
                # expanded heaps; the small helpers GetAllocSize/GetFreeSize stay calls, so
                # no auto-inlining here either
                Object(NonMatching, "game/wii/mem.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # the map floor collision behind cobj.cpp, 0x802BA4F4-0x802BD294
                # -i include/rvl_msl: math_api.h's out-of-line __fpclassifyf, as the Wii calls it
                Object(NonMatching, "game/wii/ctrigon.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                # CObjVec::Step between mem.cpp and cgs.cpp, file name unknown
                Object(Matching, "game/wii/cobjvec.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                Object(Equivalent, "game/wii/cpadact.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # the out-of-line CCamera::SetAngle(const Vec&) copy plus a CThread global
                # that nothing references - a port leftover
                Object(Matching, "game/wii/ccamera.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                Object(Matching, "game/wii/nandfile.cpp", extra_cflags=["-Cpp_exceptions on"]),
                # retail has no call to StartWiiSaveJob (declared inline, inlined into its
                # callers) while BuildWiiSaveBanner and CheckWiiSaveBanner are reached via
                # bl; -inline auto keeps both as calls too (the unit builds the same under noauto)
                Object(NonMatching, "game/wii/savemanager.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # shared GameCube game units ported to Wii (Equivalent because the class vtables sit in a separate split)
                Object(Equivalent, "game/cinterpolationeaseout.cpp", source="game/wii/cinterpolationeaseout.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # the global's __sinit stores the vtable and __construct_arrays mGroups[256] at _2034
                # (retail 0x801542A0); CCameraActionAreaEx's layout is shared with the GameCube
                Object(Matching, "game/wii/ccameraactionareaexinit.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # the Wii file queries the player position through the vtable GetPosition and calls
                # FindEnabledCameraAreaContainingPoint instead of open-coding the record loop; the
                # DrawGs debug draws and the corner-Vec extern stay GameCube-only
                Object(NonMatching, "game/ccameraactionareaex.cpp", source="game/wii/ccameraactionareaex.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # chibi robo's actions, the GameCube's objb_act.cpp: retail 0x800F48E0-0x801067C0 holds its
                # functions in the reverse order, GetTargetFloorPosition first and UpdatePushedObject last
                # (ResetTumbleAction, the GameCube's former xacttumble.cpp part, follows); -i include/rvl_msl for the
                # nw4r sound headers game/wii/xobjflags.h and csound.h pull in
                Object(NonMatching, "game/wii/objb_act.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-i include/rvl_msl"]),
                # the tumble action's front half, 0x801067C0-0x80106D60: ResetTumbleAction (the
                # GameCube's former xacttumble.cpp part of objb_act.cpp), FinishTumbleAction and HandleTumbleActionInput;
                # UpdatePlayerTumbleAnimation follows, still to port
                Object(NonMatching, "game/wii/xacttumble.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                # the tumble fade and the floor ring (the GameCube's objb_act.cpp, former xactradar.cpp tail, retail
                # 0x801078C8-0x80107C50); HandleCopterActionInput and the rest of Nr. 68 follow
                Object(NonMatching, "game/wii/xactradar.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                # the attachment objects (copter, popper, radar, OX mark), retail
                # 0x8010E548-0x8010F944: the action code calls their anim helpers with plain
                # bl, so they were a separate file; in xactradar.cpp -ipa file inlines them
                Object(NonMatching, "game/wii/xobjattachment.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                # the whole cord in one unit on the Wii (retail 0x80110C2C-0x80111ADC): nodes, the
                # draw entry that takes the node block, InitPlugCord's function-scope static point
                # array and ResetPlugCordPoints share one literal pool
                Object(NonMatching, "game/wii/xobjplug.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                Object(NonMatching, "game/wii/xobjplugcord.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # on Wii the two destructors live alone here; CamAreaSlot's ctor is emitted as the
                # first function of ccameraactionarea.cpp (retail 0x80154310)
                Object(Matching, "game/wii/ccameraactionareaexdtor.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # the area and map camera halves are two retail files, each with its own __FILE__ string pool
                # -i include/rvl_msl: nw4r::math's VEC3Dot, which the Wii inlines where the GameCube called PSVECDotProduct
                Object(NonMatching, "game/wii/ccameraactionarea.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(NonMatching, "game/wii/ccameraactionmap2.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(Matching, "game/gameosreport.cpp", source="game/wii/gameosreport.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                Object(Matching, "game/cpaddtor.cpp", source="game/wii/cpaddtor.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # Wii-only verified members: fade state and the texture matrices live four
                # bytes later than the GameCube layout (WiiFbTexture is one word bigger)
                Object(Equivalent, "game/gamefbsubwinfade.cpp", source="game/wii/gamefbsubwinfade.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # Wii CCamera vtable: _4C SetInterest(Vec&)/_58 SetAngleY/_5C SetAngleX/_44 SetDistance/
                # _18 SetFov/_70 CalcEye -- mCamera embedded at FbSubwin+0x148
                Object(Matching, "game/gamefbsubwincam.cpp", source="game/wii/gamefbsubwincam.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                Object(NonMatching, "game/gameinit.cpp", source="game/wii/gameinit.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # the XObj core methods, 0x800AC07C-0x800B1558:
                # init/reset, the draw dispatch virtuals, the screen-list queue pair and
                # ClearAnim are written; load, release, update, the material helpers and
                # the floor test still come from the retail asm
                # -i include/rvl_msl like the other xobj units
                Object(NonMatching, "game/wii/obja.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                # the XObj floors, projections and event virtuals, 0x800B1558-0x800BD6B4
                #; the GameCube's cjanim.cpp plus obja.cpp's and
                # xobjfloor.cpp's floor helpers - the stepper, callbacks and __sinit
                # still come from the retail asm
                Object(NonMatching, "game/wii/cjanim.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # retail xobj_ut.cpp, 0x800BD6B4-0x800C659C: object pools, member and pool-subclass
                # ctors/dtors (includes xobjmember.cpp and xobjctor.cpp), stage spawn, reader threads,
                # and XAction base
                Object(NonMatching, "game/wii/xobj_ut.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                # the event script runner, 0x8009453C-0x800A18B8
                # -i include/rvl_msl: the nw4r::ef handles of the event effects
                Object(NonMatching, "game/wii/evt.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                # the scene camera file, 0x800C659C-0x800C7DC0: the CCamera
                # virtuals defined here, XObjScnCam's overrides, XObjLight's and XObjMap's
                # ResetObjectState and the CustomShadow pair; the big decodes
                # (fn_800C66C8, fn_800C6CF0, fn_800C7764) still come from the retail asm
                # -i include/rvl_msl like the other xobj units
                Object(NonMatching, "game/wii/xobj_camera.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                # the map object, 0x800C7DC0-0x800C9E84: XObjMap's overrides
                # are written; the record-list helpers and the scene-proc draw callback still come
                # from the retail asm
                # -i include/rvl_msl like the other xobj units
                Object(NonMatching, "game/wii/xobj_map.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                # the player object, 0x800C9E84-0x800E9D30
                Object(NonMatching, "game/wii/xobj_chr.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                Object(NonMatching, "game/wii/ringmenu.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                Object(NonMatching, "game/wii/xobjchrmenu.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # the suit-change trail and the HUD point particles, 0x800F0100-0x800F48E0
                #; the retail rodata says xobj_chr.cpp
                Object(NonMatching, "game/wii/efftrail.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # the helpers the GameCube file inlines stay calls in retail: no auto-inlining
                Object(NonMatching, "game/wii/cprimitive.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # -inline noauto like the engine files around it (cfile, ctpl, THPSimple): Update
                # calls Stop where auto-inlining would expand it
                Object(Equivalent, "game/cmovie.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-inline noauto"]),
                # member offsets are shared with the GameCube class (mEyeInterps _18C, mFovInterps
                # _330, the interp pointers _44-_54); the Wii vtable re-orders the Vec& overloads
                Object(NonMatching, "game/ccameraactionevt.cpp", source="game/wii/ccameraactionevt.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # Wii layout: no mYaw, drag pan over gWiiPointer, four edge quads and a
                # CScrnWipeTile at _284 whose vptr sits behind CScrnColor's data at _2B4
                Object(NonMatching, "game/wii/ccameraactionscroll2.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # shared class layout with the GameCube; the Wii file goes through the player
                # vtable (GetMtxPtr _AC, GetCameraTarget _1B0) and reads the remote's direction
                # bits from gWiiPointer instead of an HSD pad
                Object(NonMatching, "game/wii/ccameraactionchase.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # the Wii class is larger (pointer tracker and two response curves); the
                # remote's pointer aims the look while the GameCube file is stick-only
                Object(NonMatching, "game/wii/ccameraactioneye.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # SetObj keeps its signature but the Wii body always clears the binding; the camera's
                # vectors sit in CCamera::mView here
                Object(Matching, "game/wii/ccameraactionanim.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # member offsets shared with the GameCube class (mTarget _40, mInterestMode _58);
                # the Wii file runs the functions in the binary's own order and goes through the
                # Wii player vtable and CCamera::mView where the GameCube used XObj and mCamera
                Object(NonMatching, "game/wii/ccameraactionreset.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # member offsets shared with the GameCube classes; the Wii file consumes the
                # remote's dpad bits on gWiiPointer.mFlags and the player Y rotation comes
                # through a vtable call instead of HSD_JObjGetRotationY
                Object(Matching, "game/wii/ccameraactionscroll.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # the Wii camera is the base subobject, so the GameCube control fields sit 0x64
                # higher; the remote's flag words replace the HSD pad reads and the Wii vtable
                # keeps only CCamera's functions, so the class adds no virtuals of its own
                Object(NonMatching, "game/wii/cmotioncontrolcamera.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # the Wii Params layout matches the GameCube class (mRank _08, mMoney _14,
                # mScrap _18, mInventory _3C); the HUD callbacks it calls are empty stubs.
                # the Wii file carries the functions in the Wii's order: the GameCube file's
                # order is the exact reverse, so a shared source cannot link here
                Object(Matching, "game/wii/params.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # same Inventory layout on both; the Wii file carries the functions in the
                # Wii's order (exact reverse of the GameCube file). RemoveInventoryItemCount
                # re-reads mCount for `count` instead of subtracting in one expression
                Object(NonMatching, "game/wii/inventory.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # unit 77: the sound, flag-backed pickup and hbox-forward functions; the
                # GameCube's xobjcoinupdate.cpp covers only the coin half. -i include/rvl_msl:
                # game/wii/xobjflags.h's AnimSfx member pulls the nw4r sound headers
                Object(NonMatching, "game/wii/xobjcoinupdate.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                # Wii XObjHBox: the carried part is an embedded plain XObj (_9B0) behind the
                # 155-slot chain vtable; the Wii file carries the GameCube xobjhboxpart and
                # xobjhboxupdate functions in retail order. NonMatching for now: the shared
                # literal pool (lbl_805A54xx) is owned by neighbouring auto objects
                Object(NonMatching, "game/wii/xobjhboxpart.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # unit 79: the box's screen overlay and hit handling plus the dust pile's
                # flag/reset/placement half. -i include/rvl_msl: game/wii/xobjflags.h's
                # AnimSfx member pulls the nw4r sound headers
                Object(NonMatching, "game/wii/xobjhbox.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                # unit 80: the dust pile's update, event and billboard half; the event wait
                # (UpdateDustEvent) and the shared billboard body (fn_80118058) are free-standing,
                # SetAnim a passthrough to XObj::SetAnim. -i include/rvl_msl: xobjflags.h
                Object(NonMatching, "game/wii/xobjdust.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                # unit 81: the dust pile's item/hit reactions and the free object's state
                # half. -i include/rvl_msl: game/wii/xobjflags.h's AnimSfx member pulls the
                # nw4r sound headers
                Object(NonMatching, "game/wii/xobjfreeevent.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                # unit 82: the free object's burst update and hit overlay, the coin's two
                # sound fades (vtable _20C/_210) and the three per-kind collection flags its
                # __sinit registers. -i include/rvl_msl: xobjflags.h as above
                Object(NonMatching, "game/wii/xobjfreeupdate.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                # unit 83: the rising lifts and their camera action. GameCube layout
                # shifted 0x28 behind the bigger XObj; the camera's Update, HandleEventCommand
                # and a few small helpers are not ported yet. -i include/rvl_msl: AnimSfx
                Object(NonMatching, "game/wii/objb_leticker.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(NonMatching, "game/wii/objb_army.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                # Wii XObj family: no sysdolphin head, nw4r callback base in front (+0x28). The
                # Wii file carries the GameCube file's functions in exact reverse order. NonMatching:
                # its 0.0f literal is pooled across TUs at lbl_805A58B0 -- a compiled unit can not
                # export that name for the eight neighbouring auto objects, and claiming the literal
                # in splits would take it away from the auto object that defines it
                # the spiders, one retail file each (0x8012063C, 0x80128C84 and 0x80132330 on): the Wii keeps the
                # GameCube files' functions in reverse order, ~XObjSpider and the ctors sit in the pool ctor block,
                # and the small spider's status table functions follow its __sinit inside its file
                Object(NonMatching, "game/wii/xobjspider.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                Object(NonMatching, "game/wii/xobjspidersmall.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                Object(NonMatching, "game/wii/xobjspiderboss.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                Object(NonMatching, "game/wii/xobjactor.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                # XObjActor3 RTTI and Kind0..Kind19 identify the GameCube counterpart;
                # recover the unit in function-sized splits, starting with Kind10.
                Object(NonMatching, "game/wii/xobjactor3.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(NonMatching, "game/wii/xobjactortoy.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(NonMatching, "rel/stage02/fighter.cpp", source="rel/wii/stage02/fighter.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(NonMatching, "rel/stage02/car.cpp", source="rel/wii/stage02/car.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(NonMatching, "rel/stage02/rouka.cpp", source="rel/wii/stage02/rouka.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(NonMatching, "rel/stage03/decapanel.cpp", source="rel/wii/stage03/decapanel.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(NonMatching, "rel/stage20/stage20.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(NonMatching, "rel/stage16/ufo.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(NonMatching, "rel/stage17/stage17.cpp", source="rel/wii/stage17/stage17.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(NonMatching, "rel/stage11/haisui.cpp", source="rel/wii/stage11/haisui.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(NonMatching, "rel/stage18/kako.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(NonMatching, "rel/stage08/stage08.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(NonMatching, "rel/stage09/ambient.cpp", source="rel/wii/stage09/ambient.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(NonMatching, "rel/stage04/jenny.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(NonMatching, "rel/stage06/parent.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(NonMatching, "rel/stage07/living.cpp", source="rel/wii/stage07/living.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(NonMatching, "rel/fafa.cpp", source="rel/wii/fafa.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(NonMatching, "rel/ana.cpp", source="rel/wii/ana.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(NonMatching, "rel/trash.cpp", source="rel/wii/trash.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(NonMatching, "rel/stage01/hamburg.cpp", source="rel/wii/stage01/hamburg.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(NonMatching, "rel/stage01/hamburgcmd.cpp", source="rel/wii/stage01/hamburgcmd.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(NonMatching, "game/wii/xobjactorjenny.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(NonMatching, "game/wii/xobjactormama.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(NonMatching, "game/wii/xobjactorpapa.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(NonMatching, "game/wii/xobjactortao.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                Object(NonMatching, "game/wii/xobjactordeca.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                # the desc struct is kept under the HSD_ImageDesc name (retail stores at +0x800
                # match its field layout); no sysdolphin on Wii
                Object(Equivalent, "game/ctoon.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on"]),
                # the head of the Wii game file (retail 0x80017BC0-0x80019A68): main, CBase's loop, its
                # empty virtuals, the drive and reset watch and operator new; Start, the corner icon
                # and the two system-message virtuals have no source yet
                Object(NonMatching, "game/wii/gamemain.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                # the game's operator delete, in a file of its own: it frees into the MEM1 exp heap
                # (retail 0x80019A68 loads the InitMem1 global and calls MEMFreeToExpHeap, no null check)
                Object(Matching, "game/wii/delete.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # the Wii TU 0x80019AB4-0x80019BA4: the game's operator
                # new[]/delete[] pair on the MEM1 heap plus the aligned operator new[] on the
                # game heap, emitted new[], delete[], new[](aligned)
                Object(Matching, "game/wii/new.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # Wii TU 0x8001A958-0x8001BA54: FbSubwin::Copy and FbSubwin::Draw,
                # ported to the Wii CGs member setters; Draw is still off
                Object(NonMatching, "game/wii/fbsubwin.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                Object(NonMatching, "game/wii/gxb.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                Object(NonMatching, "game/wii/fbmirror.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file"]),
                # Wii TU 0x8001C2D8-0x8002CFB8 (retail game.cpp update + draw halves combined): CGame's new game
                # reset, BeforeUpdate, Update, Draw, stage change, sound pause and movie code
                Object(NonMatching, "game/wii/gameupdate.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
                # retail has extab entries for the three functions and emits them in reverse (_prolog first):
                # the game group's compiler with exceptions on and deferred inlining gives both. The object
                # is byte-exact, but nothing calls _epilog and _unresolved: retail kept them through its
                # linker script (the REL entry points), the linker here strips them, so it stays unlinked.
                Object(Equivalent, "rel/module.c", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on"]),
                # the Wii game TU tail 0x8002CFB8-0x800354DC: ~CGame, ~CPadEx, emitted member
                # destructors, camera action/interpolation methods, gGame, and static initializer.
                # -DNW4R_ASSERT: retail keeps nw4r::db::Panic assertions in the inline g3d/snd helpers
                # instantiated by this TU (matching the retail panic strings in .rodata).
                Object(NonMatching, "game/wii/cinterpolationeaseoutdivide.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-DNW4R_ASSERT"]),
                # the Wii sound file 0x80036694-0x8003CEB0: the sound front end,
                # the sound banks, the sound picks, the glyph sounds, the stage reverb and AnimSfx. The
                # nw4r sound headers need the Wii MSL's <cstdarg> and <new>
                Object(NonMatching, "game/wii/sound.cpp", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions on", "-ipa file", "-i include/rvl_msl"]),
            ],
        },
        {
            # Broadcom's Bluetooth stack (BTE) after wii-ipl's reconstruction (identical function sizes in all
            # 732 functions). Sources include their headers by bare name, hence the include path.
            # Two compilers, as in wii-ipl: the stack itself (bd.c onwards) is GC/3.0a5, the Wii port layer
            # (gki_*, hcisu_h2, uusb_ppc, the *_cfg tables, bte_*, btu_task1) GC/3.0a5.2 like the rest of
            # the SDK. Measured on our units: all on 3.0a5.2 leaves btm_inq, l2c_link, rfc_ts_frames and
            # sdp_main with one or two functions open each, all on 3.0a5 costs a function in uusb_ppc.
            "lib": "BTE",
            "mw_version": "GC/3.0a5",
            "cflags": [*wii_cflags, "-Cpp_exceptions off", "-fp_contract off", "-common off", "-ipa file", "-use_lmw_stmw off", "-i include/revolution/BTE"],
            "progress_category": "sdk",
            "objects": [
                Object(Matching, "revolution/BTE/gki_buffer.c", mw_version="GC/3.0a5.2"),
                Object(Matching, "revolution/BTE/gki_time.c", mw_version="GC/3.0a5.2"),
                Object(Matching, "revolution/BTE/gki_ppc.c", mw_version="GC/3.0a5.2"),
                Object(Matching, "revolution/BTE/hcisu_h2.c", mw_version="GC/3.0a5.2"),
                Object(Matching, "revolution/BTE/uusb_ppc.c", mw_version="GC/3.0a5.2"),
                Object(Matching, "revolution/BTE/bta_dm_cfg.c", mw_version="GC/3.0a5.2"),
                Object(Matching, "revolution/BTE/bta_hh_cfg.c", mw_version="GC/3.0a5.2"),
                Object(Matching, "revolution/BTE/bta_sys_cfg.c", mw_version="GC/3.0a5.2"),
                Object(Matching, "revolution/BTE/bte_hcisu.c", mw_version="GC/3.0a5.2"),
                Object(Matching, "revolution/BTE/bte_init.c", mw_version="GC/3.0a5.2"),
                Object(Matching, "revolution/BTE/bte_logmsg.c", mw_version="GC/3.0a5.2"),
                Object(Matching, "revolution/BTE/bte_main.c", mw_version="GC/3.0a5.2"),
                Object(Matching, "revolution/BTE/btu_task1.c", mw_version="GC/3.0a5.2"),
                Object(Matching, "revolution/BTE/bd.c"),
                Object(Matching, "revolution/BTE/bta_sys_conn.c"),
                Object(Matching, "revolution/BTE/bta_sys_main.c"),
                Object(Matching, "revolution/BTE/ptim.c"),
                Object(Matching, "revolution/BTE/utl.c"),
                Object(Matching, "revolution/BTE/bta_dm_act.c"),
                Object(Matching, "revolution/BTE/bta_dm_api.c"),
                Object(Matching, "revolution/BTE/bta_dm_main.c"),
                Object(Matching, "revolution/BTE/bta_dm_pm.c"),
                Object(Matching, "revolution/BTE/bta_hh_act.c"),
                Object(Matching, "revolution/BTE/bta_hh_api.c"),
                Object(Matching, "revolution/BTE/bta_hh_main.c"),
                Object(Matching, "revolution/BTE/bta_hh_utils.c"),
                Object(Matching, "revolution/BTE/btm_acl.c"),
                Object(Matching, "revolution/BTE/btm_dev.c"),
                Object(Matching, "revolution/BTE/btm_devctl.c"),
                Object(Matching, "revolution/BTE/btm_discovery.c"),
                Object(Matching, "revolution/BTE/btm_inq.c"),
                Object(Matching, "revolution/BTE/btm_main.c"),
                Object(Matching, "revolution/BTE/btm_pm.c"),
                Object(Matching, "revolution/BTE/btm_sco.c"),
                Object(Matching, "revolution/BTE/btm_sec.c"),
                Object(Matching, "revolution/BTE/btu_hcif.c"),
                Object(Matching, "revolution/BTE/btu_init.c"),
                Object(Matching, "revolution/BTE/wbt_ext.c"),
                Object(Matching, "revolution/BTE/gap_api.c"),
                Object(Matching, "revolution/BTE/gap_conn.c"),
                Object(Matching, "revolution/BTE/gap_utils.c"),
                Object(Matching, "revolution/BTE/hcicmds.c"),
                Object(Matching, "revolution/BTE/hidd_api.c"),
                Object(Matching, "revolution/BTE/hidd_conn.c"),
                Object(Matching, "revolution/BTE/hidd_mgmt.c"),
                Object(Matching, "revolution/BTE/hidd_pm.c"),
                Object(Matching, "revolution/BTE/hidh_api.c"),
                Object(Matching, "revolution/BTE/hidh_conn.c"),
                Object(Matching, "revolution/BTE/l2c_api.c"),
                Object(Matching, "revolution/BTE/l2c_csm.c"),
                Object(Matching, "revolution/BTE/l2c_link.c"),
                Object(Matching, "revolution/BTE/l2c_main.c"),
                Object(Matching, "revolution/BTE/l2c_utils.c"),
                Object(Matching, "revolution/BTE/port_api.c"),
                Object(Matching, "revolution/BTE/port_rfc.c"),
                Object(Matching, "revolution/BTE/port_utils.c"),
                Object(Matching, "revolution/BTE/rfc_l2cap_if.c"),
                Object(Matching, "revolution/BTE/rfc_mx_fsm.c"),
                Object(Matching, "revolution/BTE/rfc_port_fsm.c"),
                Object(Matching, "revolution/BTE/rfc_port_if.c"),
                Object(Matching, "revolution/BTE/rfc_ts_frames.c"),
                Object(Matching, "revolution/BTE/rfc_utils.c"),
                Object(Matching, "revolution/BTE/sdp_api.c"),
                Object(Matching, "revolution/BTE/sdp_db.c"),
                Object(Matching, "revolution/BTE/sdp_discovery.c"),
                Object(Matching, "revolution/BTE/sdp_main.c"),
                Object(Matching, "revolution/BTE/sdp_server.c"),
                Object(Matching, "revolution/BTE/sdp_utils.c"),
            ],
        },
        {
            # NintendoWare for Revolution (nw4r::ut, db, math ...), retail 0x80234080-0x802A5358.
            # Compiler: the libraries' version strings carry CodeWarrior build 0x4199_60831, GC/3.0a5.2.
            # Flags: no stmw/lmw anywhere in the block (-use_lmw_stmw off), no RTTI records beside the
            # vtables (-RTTI off), -ipa file and -fp_contract off as every reference decomp builds NW4R
            # (xenoblade, pik2wii, wiiplay, ogws).
            "lib": "nw4r",
            "mw_version": "GC/3.0a5.2",
            "cflags": [
                "-i include/rvl_msl", *wii_cflags, "-Cpp_exceptions off", "-RTTI off", "-fp_contract off", "-ipa file",
                "-use_lmw_stmw off", "-enc SJIS",
            ],
            "progress_category": "nw4r",
            "objects": [
                Object(Matching, "nw4r/ut/ut_list.cpp"),
                Object(Matching, "nw4r/ut/ut_LinkList.cpp"),
                Object(Matching, "nw4r/ut/ut_binaryFileFormat.cpp"),
                Object(Matching, "nw4r/ut/ut_CharStrmReader.cpp"),
                Object(Matching, "nw4r/ut/ut_IOStream.cpp"),
                Object(Matching, "nw4r/ut/ut_FileStream.cpp"),
                Object(Matching, "nw4r/ut/ut_DvdFileStream.cpp"),
                Object(Matching, "nw4r/ut/ut_DvdLockedFileStream.cpp"),
                Object(Matching, "nw4r/ut/ut_LockedCache.cpp"),
                Object(Matching, "nw4r/ut/ut_Font.cpp"),
                Object(Matching, "nw4r/ut/ut_RomFont.cpp"),
                Object(Matching, "nw4r/ut/ut_ResFontBase.cpp"),
                Object(Matching, "nw4r/ut/ut_ResFont.cpp"),
                # nw4r::db (0x80235D54-0x802362B8)
                Object(Matching, "nw4r/db/db_console.cpp"),
                Object(NonMatching, "nw4r/db/db_assert.cpp"),
                Object(NonMatching, "nw4r/math/math_arithmetic.cpp"),
                Object(NonMatching, "nw4r/math/math_triangular.cpp"),
                Object(NonMatching, "nw4r/math/math_types.cpp"),
                # code 5 of 5; retail's .sdata2 starts with 0.5f and 0.0f, here they come after the two
                # TanDeg constants: a function ahead of FRUSTUM::Set that the linker removed used them
                # (the reference decomps force the order with a macro)
                Object(Equivalent, "nw4r/math/math_geometry.cpp"),
                # nw4r::ef (0x802362B8-0x8025B5A4), version text of 23.1.2009; after wiiplay's older ef (8.6.2007)
                # emitter, util and smooth stripe have 100 % code but their literal pools differ from
                # retail (emitter lacks the 0.0f and NaN of functions the linker removed, util and postfield
                # in another order, smooth stripe .sdata2/.sbss); animcurve, postfield and particlemanager
                # have open functions (see // NONMATCHING in the files).
                Object(Matching, "nw4r/ef/ef_draworder.cpp"),
                Object(Matching, "nw4r/ef/ef_effect.cpp"),
                Object(Matching, "nw4r/ef/ef_effectsystem.cpp"),
                Object(Equivalent, "nw4r/ef/ef_emitter.cpp"),
                Object(NonMatching, "nw4r/ef/ef_animcurve.cpp"),
                Object(NonMatching, "nw4r/ef/ef_postfield.cpp"),
                Object(NonMatching, "nw4r/ef/ef_particle.cpp"),
                Object(NonMatching, "nw4r/ef/ef_particlemanager.cpp"),
                Object(Matching, "nw4r/ef/ef_resource.cpp"),
                Object(Equivalent, "nw4r/ef/ef_util.cpp"),
                Object(Matching, "nw4r/ef/ef_handle.cpp"),
                Object(NonMatching, "nw4r/ef/ef_emitterform.cpp"),
                Object(Matching, "nw4r/ef/ef_creationqueue.cpp"),
                Object(Matching, "nw4r/ef/emform/ef_emform.cpp"),
                Object(Matching, "nw4r/ef/emform/ef_point.cpp"),
                Object(Matching, "nw4r/ef/emform/ef_line.cpp"),
                Object(Matching, "nw4r/ef/emform/ef_disc.cpp"),
                Object(Matching, "nw4r/ef/emform/ef_sphere.cpp"),
                Object(Matching, "nw4r/ef/emform/ef_cylinder.cpp"),
                Object(Matching, "nw4r/ef/emform/ef_torus.cpp"),
                Object(NonMatching, "nw4r/ef/emform/ef_cube.cpp"),
                Object(Matching, "nw4r/ef/drawstrategy/ef_drawstrategybuilder.cpp"),
                Object(NonMatching, "nw4r/ef/drawstrategy/ef_drawstrategyimpl.cpp"),
                Object(NonMatching, "nw4r/ef/drawstrategy/ef_drawbillboardstrategy.cpp"),
                Object(NonMatching, "nw4r/ef/drawstrategy/ef_drawdirectionalstrategy.cpp"),
                Object(Matching, "nw4r/ef/drawstrategy/ef_drawfreestrategy.cpp"),
                Object(NonMatching, "nw4r/ef/drawstrategy/ef_drawlinestrategy.cpp"),
                Object(Matching, "nw4r/ef/drawstrategy/ef_drawpointstrategy.cpp"),
                Object(NonMatching, "nw4r/ef/drawstrategy/ef_drawstripestrategy.cpp"),
                Object(NonMatching, "nw4r/ef/drawstrategy/ef_drawsmoothstripestrategy.cpp"),
                Object(Matching, "nw4r/ef/ef_res_emitter_ac.cpp"),
                # nw4r::g3d (0x8025B5A4-0x80282CA0)
                Object(Matching, "nw4r/g3d/res/g3d_rescommon.cpp"),
                Object(Matching, "nw4r/g3d/res/g3d_resdict.cpp"),
                Object(Matching, "nw4r/g3d/res/g3d_resfile.cpp"),
                Object(Matching, "nw4r/g3d/res/g3d_resmdl.cpp"),
                Object(Matching, "nw4r/g3d/res/g3d_resshp.cpp"),
                # 11 of 12: SetNumTevStages expands GXSetTevOrder, which retail calls (see the comment in the source).
                Object(NonMatching, "nw4r/g3d/res/g3d_restev.cpp"),
                Object(Matching, "nw4r/g3d/res/g3d_resmat.cpp"),
                Object(Matching, "nw4r/g3d/res/g3d_resvtx.cpp"),
                Object(Matching, "nw4r/g3d/res/g3d_restex.cpp"),
                Object(Matching, "nw4r/g3d/res/g3d_resnode.cpp"),
                Object(NonMatching, "nw4r/g3d/res/g3d_resanm.cpp"),
                Object(Matching, "nw4r/g3d/res/g3d_resanmvis.cpp"),
                Object(Matching, "nw4r/g3d/res/g3d_resanmclr.cpp"),
                Object(Matching, "nw4r/g3d/res/g3d_resanmtexpat.cpp"),
                Object(Matching, "nw4r/g3d/res/g3d_resanmtexsrt.cpp"),
                # -fp_contract on for this unit: retail contracts the multiply-adds of CalcAnimationFVS (three
                # instances) and GetAnmRotation (the unit has 58 fmadds/fmsubs, first at 0x80262A2C); with -fp_contract off
                # these four are 0x10 to 0x84 bytes longer and nothing else in the unit changes. xenoblade's
                # g3d_resanmchr.cpp starts with the same setting as a pragma.
                Object(NonMatching, "nw4r/g3d/res/g3d_resanmchr.cpp", extra_cflags=["-fp_contract on"]),
                Object(Matching, "nw4r/g3d/res/g3d_reslightset.cpp"),
                Object(Matching, "nw4r/g3d/res/g3d_resanmamblight.cpp"),
                Object(Matching, "nw4r/g3d/res/g3d_resanmlight.cpp"),
                Object(Matching, "nw4r/g3d/res/g3d_resanmfog.cpp"),
                Object(Matching, "nw4r/g3d/res/g3d_resanmcamera.cpp"),
                Object(Matching, "nw4r/g3d/res/g3d_resanmscn.cpp"),
                Object(Matching, "nw4r/g3d/res/g3d_resanmshp.cpp"),
                Object(NonMatching, "nw4r/g3d/g3d_transform.cpp"),
                Object(Matching, "nw4r/g3d/g3d_anmvis.cpp"),
                Object(Matching, "nw4r/g3d/g3d_anmclr.cpp"),
                Object(Matching, "nw4r/g3d/g3d_anmtexpat.cpp"),
                Object(Matching, "nw4r/g3d/g3d_anmtexsrt.cpp"),
                Object(NonMatching, "nw4r/g3d/g3d_anmchr.cpp"),
                Object(Matching, "nw4r/g3d/g3d_anmshp.cpp"),
                Object(NonMatching, "nw4r/g3d/g3d_anmscn.cpp"),
                # 5 of 5, not linked: retail emits G3dObj's weak virtuals as IsDerivedFrom, GetTypeName, GetTypeObj
                # (0x8026C5EC), this source as GetTypeName, GetTypeObj, IsDerivedFrom. xenoblade gets retail's
                # order with a DECOMP_FORCEACTIVE reference to IsDerivedFrom at the end of the file, wiiplay and ogws
                # too. The compiler emits a class's weak virtuals in reverse order of declaration; defining
                # IsDerivedFrom inline behind the class instead of inside it changes nothing.
                Object(NonMatching, "nw4r/g3d/g3d_obj.cpp"),
                Object(NonMatching, "nw4r/g3d/g3d_anmobj.cpp"),
                Object(Matching, "nw4r/g3d/platform/g3d_gpu.cpp"),
                Object(NonMatching, "nw4r/g3d/platform/g3d_cpu.cpp"),
                Object(Matching, "nw4r/g3d/g3d_state.cpp"),
                Object(Matching, "nw4r/g3d/g3d_draw1mat1shp.cpp"),
                Object(NonMatching, "nw4r/g3d/g3d_calcview.cpp"),
                Object(Matching, "nw4r/g3d/g3d_dcc.cpp"),
                Object(Matching, "nw4r/g3d/g3d_workmem.cpp"),
                Object(NonMatching, "nw4r/g3d/g3d_calcworld.cpp"),
                Object(Matching, "nw4r/g3d/g3d_draw.cpp"),
                Object(NonMatching, "nw4r/g3d/g3d_camera.cpp"),
                Object(Matching, "nw4r/g3d/dcc/g3d_basic.cpp"),
                Object(Matching, "nw4r/g3d/dcc/g3d_maya.cpp"),
                Object(Matching, "nw4r/g3d/dcc/g3d_xsi.cpp"),
                Object(Matching, "nw4r/g3d/dcc/g3d_3dsmax.cpp"),
                Object(Matching, "nw4r/g3d/g3d_scnobj.cpp"),
                Object(Matching, "nw4r/g3d/g3d_scnroot.cpp"),
                Object(Matching, "nw4r/g3d/g3d_scnmdlsmpl.cpp"),
                Object(Matching, "nw4r/g3d/g3d_scnmdl.cpp"),
                Object(Matching, "nw4r/g3d/g3d_calcmaterial.cpp"),
                Object(Matching, "nw4r/g3d/g3d_init.cpp"),
                Object(Matching, "nw4r/g3d/g3d_scnproc.cpp"),
                Object(NonMatching, "nw4r/g3d/g3d_fog.cpp"),
                Object(Matching, "nw4r/g3d/g3d_light.cpp"),
                Object(NonMatching, "nw4r/g3d/g3d_calcvtx.cpp"),
                # nw4r::snd (0x80283EA8-0x802A5358)
                Object(Matching, "nw4r/snd/snd_AxManager.cpp"),
                Object(Matching, "nw4r/snd/snd_AxVoice.cpp"),
                Object(Matching, "nw4r/snd/snd_AxVoiceManager.cpp"),
                Object(Matching, "nw4r/snd/snd_AxfxImpl.cpp"),
                Object(Matching, "nw4r/snd/snd_Bank.cpp"),
                Object(Matching, "nw4r/snd/snd_BankFile.cpp"),
                Object(Matching, "nw4r/snd/snd_BasicPlayer.cpp"),
                # code 40 of 40 by content; Stop, Pause and IsPause are StopWiiSfxTrack, SetWiiSfxTrackPaused and
                # IsWiiSfxTrackPaused in symbols.txt, the names src/game/wii/msg.cpp calls
                Object(Matching, "nw4r/snd/snd_BasicSound.cpp"),
                Object(Matching, "nw4r/snd/snd_BiquadFilterPreset.cpp"),
                Object(Matching, "nw4r/snd/snd_Channel.cpp"),
                Object(Matching, "nw4r/snd/snd_DisposeCallbackManager.cpp"),
                Object(Matching, "nw4r/snd/snd_DvdSoundArchive.cpp"),
                Object(Matching, "nw4r/snd/snd_EnvGenerator.cpp"),
                Object(Matching, "nw4r/snd/snd_ExternalSoundPlayer.cpp"),
                Object(Matching, "nw4r/snd/snd_FrameHeap.cpp"),
                Object(Matching, "nw4r/snd/snd_FxReverbHiDpl2.cpp"),
                Object(Matching, "nw4r/snd/snd_InstancePool.cpp"),
                Object(Matching, "nw4r/snd/snd_Lfo.cpp"),
                Object(Matching, "nw4r/snd/snd_MidiSeqPlayer.cpp"),
                Object(Matching, "nw4r/snd/snd_MmlParser.cpp"),
                Object(Matching, "nw4r/snd/snd_MmlSeqTrack.cpp"),
                Object(Matching, "nw4r/snd/snd_MmlSeqTrackAllocator.cpp"),
                Object(Matching, "nw4r/snd/snd_PlayerHeap.cpp"),
                Object(Matching, "nw4r/snd/snd_RemoteSpeaker.cpp"),
                Object(Matching, "nw4r/snd/snd_RemoteSpeakerManager.cpp"),
                Object(Matching, "nw4r/snd/snd_SeqFile.cpp"),
                Object(Matching, "nw4r/snd/snd_SeqPlayer.cpp"),
                # code complete; calls BasicSound::Stop (StopWiiSfxTrack in symbols.txt)
                Object(Matching, "nw4r/snd/snd_SeqSound.cpp"),
                Object(Matching, "nw4r/snd/snd_SeqSoundHandle.cpp"),
                Object(Matching, "nw4r/snd/snd_SeqTrack.cpp"),
                Object(NonMatching, "nw4r/snd/snd_Sound3DActor.cpp"),
                Object(NonMatching, "nw4r/snd/snd_Sound3DCalculator.cpp"),
                Object(Matching, "nw4r/snd/snd_Sound3DEngine.cpp"),
                Object(NonMatching, "nw4r/snd/snd_Sound3DListener.cpp"),
                Object(Matching, "nw4r/snd/snd_Sound3DManager.cpp"),
                Object(Matching, "nw4r/snd/snd_SoundActor.cpp"),
                Object(Matching, "nw4r/snd/snd_SoundArchive.cpp"),
                Object(Matching, "nw4r/snd/snd_SoundArchiveFile.cpp"),
                Object(Matching, "nw4r/snd/snd_SoundArchiveLoader.cpp"),
                # links byte-identical; objdiff's .sdata2 below 100 % is SortPriorityList's three
                # unreferenced TMP_NUM constants, which the linker strips
                Object(Matching, "nw4r/snd/snd_SoundArchivePlayer.cpp"),
                Object(Matching, "nw4r/snd/snd_SoundHandle.cpp"),
                Object(Matching, "nw4r/snd/snd_SoundHeap.cpp"),
                Object(Matching, "nw4r/snd/snd_SoundPlayer.cpp"),
                Object(Matching, "nw4r/snd/snd_SoundStartable.cpp"),
                Object(Matching, "nw4r/snd/snd_SoundSystem.cpp"),
                Object(Matching, "nw4r/snd/snd_SoundThread.cpp"),
                Object(Matching, "nw4r/snd/snd_StrmChannel.cpp"),
                Object(Matching, "nw4r/snd/snd_StrmFile.cpp"),
                Object(Matching, "nw4r/snd/snd_StrmPlayer.cpp"),
                Object(Matching, "nw4r/snd/snd_StrmSound.cpp"),
                Object(Matching, "nw4r/snd/snd_StrmSoundHandle.cpp"),
                Object(Matching, "nw4r/snd/snd_Task.cpp"),
                Object(Matching, "nw4r/snd/snd_TaskManager.cpp"),
                Object(Matching, "nw4r/snd/snd_TaskThread.cpp"),
                Object(Matching, "nw4r/snd/snd_Voice.cpp"),
                Object(Matching, "nw4r/snd/snd_VoiceManager.cpp"),
                Object(Matching, "nw4r/snd/snd_Util.cpp"),
                Object(Matching, "nw4r/snd/snd_WaveArchive.cpp"),
                Object(Matching, "nw4r/snd/snd_WaveFile.cpp"),
                Object(Matching, "nw4r/snd/snd_WaveSound.cpp"),
                Object(Matching, "nw4r/snd/snd_WaveSoundHandle.cpp"),
                Object(Matching, "nw4r/snd/snd_WsdFile.cpp"),
                Object(Matching, "nw4r/snd/snd_WsdPlayer.cpp"),
                Object(Matching, "nw4r/snd/snd_adpcm.cpp"),
            ],
        },
        {
            # homebuttonLib, the HOME Button Menu (retail 0x80203864-0x80222DEC, RVL_SDK HBM of Sep 24 2008):
            # homebutton::, its own copies of NW4R (nw4hbm::lyt, math, ut) and of MIX/SYN/SEQ.
            # Compiler: measured on ut_CharWriter (27 functions), ut_ResFont, ut_TagProcessorBase,
            # HBMFrameController, lyt_arcResourceAccessor and math_triangular: GC/3.0a5 and GC/3.0a5.2 both
            # give retail's code in every function (pik2wii builds its HBM with GC/3.0a5), Wii/1.0 and
            # Wii/1.0a (xenoblade's and ss's choice for their later HBM) lose 12 of the 27 in ut_CharWriter and
            # all of math_triangular. The version string carries build 0x4199_60831, which is GC/3.0a5.2.
            # Flags: -ipa file (without it 8 of ut_CharWriter's 27 change), -RTTI on (the RTTI records sit
            # beside the vtables in .data), -sdata 0 -sdata2 0 (the block reads no small data at all, floats
            # come from .rodata), no stmw/lmw anywhere in the block.
            "lib": "homebuttonLib",
            "mw_version": "GC/3.0a5.2",
            "cflags": [
                "-i include/homebuttonLib", "-i include/rvl_msl/msl", *wii_cflags, "-Cpp_exceptions off", "-RTTI on",
                "-fp_contract off", "-ipa file", "-use_lmw_stmw off", "-sdata 0", "-sdata2 0", "-enc SJIS",
            ],
            "progress_category": "sdk",
            "objects": [
                Object(Matching, "homebuttonLib/HBMFrameController.cpp"),
                Object(Matching, "homebuttonLib/HBMAnmController.cpp"),
                Object(Matching, "homebuttonLib/HBMGUIManager.cpp"),
                Object(Matching, "homebuttonLib/HBMController.cpp"),
                Object(Matching, "homebuttonLib/HBMRemoteSpk.cpp"),
                Object(Matching, "homebuttonLib/HBMAxSound.cpp"),
                Object(Matching, "homebuttonLib/HBMCommon.cpp"),
                Object(Matching, "homebuttonLib/HBMBase.cpp"),
                Object(Matching, "homebuttonLib/nw4hbm/lyt/lyt_animation.cpp"),
                Object(Matching, "homebuttonLib/nw4hbm/lyt/lyt_arcResourceAccessor.cpp"),
                Object(Matching, "homebuttonLib/nw4hbm/lyt/lyt_bounding.cpp"),
                Object(Matching, "homebuttonLib/nw4hbm/lyt/lyt_common.cpp"),
                Object(Matching, "homebuttonLib/nw4hbm/lyt/lyt_drawInfo.cpp"),
                Object(Matching, "homebuttonLib/nw4hbm/lyt/lyt_group.cpp"),
                Object(Matching, "homebuttonLib/nw4hbm/lyt/lyt_layout.cpp"),
                Object(NonMatching, "homebuttonLib/nw4hbm/lyt/lyt_material.cpp"),
                Object(Matching, "homebuttonLib/nw4hbm/lyt/lyt_pane.cpp"),
                Object(Matching, "homebuttonLib/nw4hbm/lyt/lyt_picture.cpp"),
                Object(Matching, "homebuttonLib/nw4hbm/lyt/lyt_resourceAccessor.cpp"),
                Object(Matching, "homebuttonLib/nw4hbm/lyt/lyt_textBox.cpp"),
                Object(NonMatching, "homebuttonLib/nw4hbm/lyt/lyt_window.cpp"),
                Object(Matching, "homebuttonLib/nw4hbm/math/math_triangular.cpp"),
                Object(Matching, "homebuttonLib/nw4hbm/ut/ut_binaryFileFormat.cpp"),
                Object(Matching, "homebuttonLib/nw4hbm/ut/ut_CharStrmReader.cpp"),
                Object(Matching, "homebuttonLib/nw4hbm/ut/ut_CharWriter.cpp"),
                Object(Matching, "homebuttonLib/nw4hbm/ut/ut_Font.cpp"),
                Object(Matching, "homebuttonLib/nw4hbm/ut/ut_LinkList.cpp"),
                Object(Matching, "homebuttonLib/nw4hbm/ut/ut_list.cpp"),
                Object(Matching, "homebuttonLib/nw4hbm/ut/ut_ResFont.cpp"),
                Object(Matching, "homebuttonLib/nw4hbm/ut/ut_ResFontBase.cpp"),
                Object(NonMatching, "homebuttonLib/nw4hbm/ut/ut_TagProcessorBase.cpp"),
                Object(Matching, "homebuttonLib/nw4hbm/ut/ut_TextWriterBase.cpp"),
                Object(Matching, "homebuttonLib/sound/mix.cpp"),
                Object(Matching, "homebuttonLib/sound/syn.cpp"),
                Object(Matching, "homebuttonLib/sound/synctrl.cpp"),
                Object(Matching, "homebuttonLib/sound/synenv.cpp"),
                Object(Matching, "homebuttonLib/sound/synmix.cpp"),
                Object(Matching, "homebuttonLib/sound/synpitch.cpp"),
                Object(Matching, "homebuttonLib/sound/synsample.cpp"),
                Object(Matching, "homebuttonLib/sound/synvoice.cpp"),
                Object(Matching, "homebuttonLib/sound/seq.cpp"),
            ],
        },
        {
            # These Wii SDK modules share the compiler profile used by New Play Control! Pikmin.
            "lib": "RevolutionSDK-2008",
            "mw_version": "GC/3.0a5.2",
            # -fp_contract off is required: AXFXReverbHiExp{,Dpl2} and AXGetLpfCoefs
            # otherwise miscompile to fmsubs (83-97% fuzzy)
            "cflags": [*wii_cflags, "-Cpp_exceptions off", "-fp_contract off", "-common off", "-ipa file", "-use_lmw_stmw off"],
            "progress_category": "sdk",
            "objects": [
                Object(Matching, "revolution/OS/OSStateFlags.c"),
                Object(Matching, "revolution/AX/AX.c"),
                Object(Matching, "revolution/OS/OSArena.c"),
                Object(Matching, "revolution/OS/OSAudioSystem.c"),
                # Retail uses _savegpr/_restgpr calls rather than inline stmw/lmw in InsertAlarm and the setters.
                Object(Equivalent, "revolution/OS/OSAlarm.c", cflags=wii_dolphin_header_cflags),
                Object(
                    Equivalent,
                    "revolution/OS/os_exception_vectors.s",
                    add_to_all=False,
                    extra_as_implicit=["include/sdk/private/os/exception_context.inc"],
                ),
                Object(Matching, "revolution/OS/OSAlloc.c"),
                Object(Equivalent, "revolution/OS/OSCache.c"),
                Object(Equivalent, "revolution/OS/os_cache_lines.s", add_to_all=False),
                Object(Equivalent, "revolution/OS/os_locked_cache.s", add_to_all=False),
                Object(Equivalent, "revolution/OS/OSContext.c"),
                Object(
                    Equivalent,
                    "revolution/OS/os_context_switch.s",
                    add_to_all=False,
                    extra_as_implicit=["include/sdk/private/os/exception_context.inc"],
                ),
                Object(
                    Equivalent,
                    "revolution/OS/os_fpu_context.s",
                    add_to_all=False,
                    extra_as_implicit=["include/sdk/private/os/exception_context.inc"],
                ),
                Object(Matching, "revolution/OS/OSError.c"),
                Object(Equivalent, "revolution/OS/OSInterrupt.c", mw_version="GC/3.0a5.2"),
                Object(Equivalent, "revolution/OS/os_interrupt_mask.s", add_to_all=False),
                Object(
                    Equivalent,
                    "revolution/OS/os_interrupt_entry.s",
                    add_to_all=False,
                    extra_as_implicit=["include/sdk/private/os/exception_context.inc"],
                ),
                # Retail ScreenReport and OSFatal call the runtime's _savegpr/_restgpr helpers
                # rather than emitting stmw/lmw; file IPA changes the unit's text.
                Object(Matching, "revolution/OS/OSFatal.c", cflags=wii_dolphin_header_noipa_cflags),
                # Retail Decode, ReadFont, ParseStringW and OSGetFontTexel call the runtime's
                # _savegpr/_restgpr helpers rather than emitting stmw/lmw; unsigned-char codegen
                # is required to reproduce the unit's text without file IPA.
                Object(Matching, "revolution/OS/OSFont.c", cflags=[*wii_dolphin_header_noipa_cflags, "-char", "unsigned"]),
                Object(Matching, "dolphin/os/OSMessage.c"),
                Object(Matching, "revolution/OS/OSMutex.c"),
                # The unit's text is only reproduced with inline lmw/stmw enabled.
                Object(Matching, "revolution/OS/OSRtc.c", cflags=wii_dolphin_header_lmw_cflags),
                # both include <dolphin/*.h>, which only resolve under include/sdk on a
                # case-sensitive filesystem - the same flags the GameCube build uses
                Object(Matching, "dolphin/os/OSStopwatch.c", cflags=cflags_sdk_2004),
                Object(Matching, "revolution/OS/os_syscall_vector.s", add_to_all=False),
                Object(Matching, "revolution/OS/OSSync.c"),
                Object(NonMatching, "revolution/OS/OSThread.c"),
                # Retail Relocate and Undo call the runtime's _savegpr/_restgpr helpers
                # rather than emitting stmw/lmw.
                Object(Matching, "revolution/OS/OSLink.c", cflags=wii_dolphin_header_cflags),
                Object(Matching, "revolution/OS/os_timebase.s", add_to_all=False),
                # Retail OSTicksToCalendarTime calls the runtime's _savegpr_26/_restgpr_26 helpers
                # rather than emitting stmw/lmw; file IPA changes the unit's text.
                Object(Matching, "revolution/OS/OSTime.c", cflags=wii_dolphin_header_noipa_cflags),
                Object(Equivalent, "revolution/OS/OSMemory.c"),
                Object(Equivalent, "revolution/OS/os_memory_mapping.s", add_to_all=False),
                Object(Matching, "revolution/OS/OSReboot.c"),
                Object(Matching, "revolution/OS/OSReset.c"),
                # Retail OSInit/OSExceptionInit call the runtime's individual-register _savegpr/_restgpr
                # helpers rather than emitting lmw/stmw; that is the retail evidence for -use_lmw_stmw off.
                Object(Equivalent, "revolution/OS/OS.c", cflags=wii_dolphin_header_cflags),
                Object(Equivalent, "revolution/OS/os_boot_fpu.s", add_to_all=False),
                Object(Equivalent, "revolution/OS/OSExec.c"),
                Object(Equivalent, "revolution/OS/os_exec_run.s", add_to_all=False),
                Object(Matching, "revolution/OS/OSUtf.c"),
                Object(Matching, "revolution/OS/OSIpc.c"),
                Object(Matching, "revolution/OS/OSStateTM.c"),
                Object(Matching, "revolution/OS/OSPlayRecord.c"),
                # Retail has no out-of-line copies of the small resource/caller helpers, but keeps
                # NWC24iRequestShutdown and NWC24iSetRtcCounter_ as calls; with the SDK-wide auto
                # inlining those two static bodies are inlined as well (NWC24iRequestShutdown is
                # 14 statements below the auto-inline limit). That unit-wide split is the retail
                # evidence for -inline noauto. The honest source still does not match exactly.
                Object(NonMatching, "revolution/OS/OSNet.c", extra_cflags=["-inline noauto"]),
                Object(Matching, "revolution/OS/OSNandbootInfo.c"),
                Object(Matching, "revolution/OS/OSPlayTime.c"),
                Object(Matching, "revolution/OS/OSLaunch.c"),
                Object(Matching, "revolution/MTX/quat.c"),
                Object(Matching, "revolution/DVD/dvdfs.c"),
                Object(Matching, "revolution/DVD/dvd.c"),
                Object(Matching, "revolution/DVD/dvderror.c"),
                Object(Matching, "revolution/DVD/dvdidutils.c"),
                Object(Matching, "revolution/DVD/dvdFatal.c"),
                Object(Matching, "revolution/DVD/dvdDeviceError.c"),
                Object(Matching, "revolution/DVD/dvd_broadway.c"),
                Object(Matching, "revolution/VI/vi.c"),
                Object(NonMatching, "revolution/VI/i2c.c"),
                Object(Matching, "revolution/VI/vi3in1.c"),
                Object(Equivalent, "revolution/BASE/ppc_registers.s", add_to_all=False),
                Object(Equivalent, "revolution/BASE/PPCArch.c"),
                Object(Equivalent, "revolution/BASE/ppc_fp_mode.s", add_to_all=False),
                Object(Equivalent, "revolution/BASE/ppc_hid4.s", add_to_all=False),
                Object(Equivalent, "revolution/BASE/ppc_register_helpers.s", add_to_all=False),
                Object(Equivalent, "revolution/DB/db.c"),
                Object(Equivalent, "revolution/DB/db_exception_entry.s", add_to_all=False),
                # The C rotation wrappers separate three runs of Gekko paired-single code.
                Object(
                    Equivalent,
                    "revolution/MTX/mtx_paired.s",
                    add_to_all=False,
                    extra_asflags=["--defsym MTX_PAIRED_SECTION=0"],
                ),
                Object(Equivalent, "revolution/MTX/mtx.c"),
                Object(
                    Equivalent,
                    "revolution/MTX/mtx_paired_rotation.s",
                    source="revolution/MTX/mtx_paired.s",
                    add_to_all=False,
                    extra_asflags=["--defsym MTX_PAIRED_SECTION=1"],
                ),
                Object(Equivalent, "revolution/MTX/mtx_rot_axis.c"),
                Object(
                    Equivalent,
                    "revolution/MTX/mtx_paired_affine.s",
                    source="revolution/MTX/mtx_paired.s",
                    add_to_all=False,
                    extra_asflags=["--defsym MTX_PAIRED_SECTION=2"],
                ),
                Object(Equivalent, "revolution/MTX/mtx_view.c"),
                # The paired-single routines share mtx44.c's compiler literal pool in retail.
                Object(Equivalent, "revolution/MTX/mtx44_paired.s", add_to_all=False),
                Object(Equivalent, "revolution/MTX/mtx44.c"),
                # GC SDK source; the Wii SDK's mtxvec unit is a different function set.
                Object(NonMatching, "dolphin/mtx/mtxvec.c"),
                # Two scalar C units separate the three runs of paired-single vector code.
                Object(
                    Equivalent,
                    "revolution/MTX/vec_paired.s",
                    add_to_all=False,
                    extra_asflags=["--defsym VEC_PAIRED_SECTION=0"],
                ),
                Object(Matching, "revolution/MTX/vec.c"),
                Object(
                    Equivalent,
                    "revolution/MTX/vec_paired_cross.s",
                    source="revolution/MTX/vec_paired.s",
                    add_to_all=False,
                    extra_asflags=["--defsym VEC_PAIRED_SECTION=1"],
                ),
                Object(Matching, "revolution/MTX/vec_reflect.c"),
                Object(
                    Equivalent,
                    "revolution/MTX/vec_paired_distance.s",
                    source="revolution/MTX/vec_paired.s",
                    add_to_all=False,
                    extra_asflags=["--defsym VEC_PAIRED_SECTION=2"],
                ),
                # Retail PADRead calls the runtime's _savegpr/_restgpr helpers rather than emitting stmw/lmw.
                Object(Matching, "revolution/PAD/Pad.c", extra_cflags=["-ipa file", "-use_lmw_stmw off"], mw_version="GC/3.0a5.2"),
                Object(Matching, "revolution/DVD/dvdqueue.c", mw_version="GC/3.0a5.2"),
                Object(Equivalent, "revolution/AI/ai.c"),
                Object(Equivalent, "revolution/AI/ai_callback_stack.s", add_to_all=False),
                # the whole file was built without instruction scheduling: with it on, all 19 functions fall (to 48-90 %)
                Object(Matching, "revolution/EXI/EXIBios.c", mw_version="GC/3.0a5.2", extra_cflags=["-schedule off"]),
                Object(Matching, "revolution/EXI/EXIUart.c", cflags=wii_dolphin_header_cflags),
                Object(Matching, "revolution/EXI/EXICommon.c", mw_version="GC/3.0a5.2"),
                # Retail SIInterruptHandler, __SITransfer, SITransfer, GetTypeCallback and SIGetType
                # call the runtime's _savegpr/_restgpr helpers rather than emitting stmw/lmw.
                Object(Matching, "revolution/SI/SIBios.c", cflags=wii_dolphin_header_cflags),
                Object(Matching, "dolphin/si/SISamplingRate.c", mw_version="GC/3.0a5.2"),
                Object(Matching, "revolution/ARC/arc.c", mw_version="GC/3.0a5.2"),
                Object(Matching, "revolution/AX/AXAux.c"),
                Object(Matching, "revolution/AXFX/AXFXHooks.c"),
                Object(Matching, "revolution/AX/AXCL.c"),
                Object(Matching, "revolution/AX/AXComp.c"),
                Object(Matching, "revolution/AX/AXAlloc.c"),
                Object(Matching, "revolution/AX/AXOut.c"),
                Object(Matching, "revolution/AX/AXProf.c"),
                Object(NonMatching, "revolution/AX/AXVPB.c"),
                Object(Matching, "revolution/AX/AXSPB.c"),
                Object(Matching, "revolution/AXFX/AXFXReverbHi.c"),
                Object(Equivalent, "revolution/AXFX/AXFXReverbHiExp.c"),
                Object(Equivalent, "revolution/AXFX/AXFXReverbHiExpDpl2.c"),
                Object(Matching, "revolution/DSP/dsp.c", cflags=wii_dolphin_header_cflags),
                Object(Matching, "revolution/DSP/dsp_debug.c", cflags=wii_dolphin_header_cflags),
                Object(Equivalent, "revolution/DSP/dsp_task.c", cflags=wii_dolphin_header_cflags),
                Object(Matching, "revolution/WENC/wenc.c"),
                Object(Matching, "revolution/EUART/euart.c"),
                Object(Matching, "revolution/FS/fs.c"),
                Object(Matching, "revolution/IPC/ipcMain.c"),
                Object(Matching, "revolution/IPC/ipcclt.c"),
                Object(Matching, "revolution/IPC/memory.c"),
                Object(Matching, "revolution/IPC/ipcProfile.c"),
                Object(Matching, "revolution/NAND/nand.c"),
                Object(Matching, "revolution/NAND/NANDOpenClose.c"),
                Object(Matching, "revolution/NAND/NANDCore.c"),
                Object(Matching, "revolution/NAND/NANDLogging.c"),
                Object(Matching, "revolution/SC/scsystem.c"),
                Object(Matching, "revolution/SC/scapi.c"),
                Object(Matching, "revolution/SC/scapi_prdinfo.c"),
                # code complete; retail's .data ends with the texts of IUSB_RegisterInsertionNotifyWithIdAsync,
                # a function the linker removed and no reference has (see the file)
                Object(Equivalent, "revolution/USB/usb.c"),
                Object(Matching, "revolution/TPL/TPL.c"),
                Object(Equivalent, "revolution/THP/THPDec.c"),
                Object(Matching, "revolution/THP/THPAudio.c"),
                Object(Equivalent, "revolution/OS/__ppc_eabi_init.cpp"),
                Object(Equivalent, "revolution/OS/os_ppc_eabi_init.s", add_to_all=False),
                # NdevExi2A (0x8017B5A4-0x8017C0A8) is built unoptimised and without inlining: all 31
                # functions of both files show -O0 code (parameters kept on the stack, locals from r31
                # down, every static helper a call); with the library's -O4,p none of them match.
                Object(Matching, "revolution/NdevExi2A/DebuggerDriver.c", extra_cflags=["-O0", "-inline off"]),
                Object(NonMatching, "revolution/NdevExi2A/exi2.c", extra_cflags=["-O0", "-inline off"]),
                Object(Matching, "revolution/ESP/esp.c"),
                # Both WPAD units take the library flags (-fp hardware): WPAD.c (76 functions) and
                # WPADHIDParser.c (25) match and link with them, so Petari's -fp off for its WPAD.c of
                # December 2007 does not apply to this build.
                Object(Matching, "revolution/WPAD/WPAD.c"),
                Object(Matching, "revolution/WPAD/WPADHIDParser.c"),
                Object(Matching, "revolution/WPAD/WPADEncrypt.c"),
                Object(Matching, "revolution/WPAD/WPADMem.c"),
                Object(Matching, "revolution/WPAD/debug_msg.c"),
                Object(Matching, "revolution/KPAD/KPAD.c"),
                # WUD (Wii remote device manager) after wii-ipl's reconstruction, brought to our September 2008
                # build from the disassembly. It includes the Bluetooth stack's headers, which include each
                # other by bare name: the extra flag is an include path, not a code generation setting.
                # WUD.c: all 86 functions match, and .bss, .sbss, .sdata and .sdata2 are in place. Not linked:
                # retail's .data ends with four strings ("Not Found", "Found the registered WBC\n", "Removed\n",
                # "Not Found WBC\n", 0x8032D6BC-0x8032D700) of a function the linker removed, and no reference
                # has that function. With a stand-in for those 0x44 bytes the unit links to an identical dol.
                Object(Equivalent, "revolution/WUD/WUD.c", extra_cflags=["-i include/revolution/BTE"]),
                Object(Matching, "revolution/WUD/WUDHidHost.c", extra_cflags=["-i include/revolution/BTE"]),
                Object(Matching, "revolution/WUD/debug_msg.c", extra_cflags=["-i include/revolution/BTE"]),
                # GX, the Wii's own sources (src/dolphin/gx is the GameCube template). Compiler and flags are
                # the library's: with them GXBump.c matched 5 of 5 named functions at once (1 of 7 as the
                # GameCube file under GC/1.2.5n) and GX as a whole went from 11 to 130 of 191 before any
                # source change. GC/3.0a3 to 3.0a5 give the same code on GXTexture.c; Wii/1.0 to 1.3 lose
                # four to five more functions there, and without -ipa file GXInitTexObj falls to 94 %.
                Object(Matching, "revolution/GX/GXInit.c", cflags=wii_gx_cflags),
                Object(NonMatching, "revolution/GX/GXFifo.c", cflags=wii_gx_cflags),
                Object(NonMatching, "revolution/GX/GXAttr.c", cflags=wii_gx_cflags),
                Object(Matching, "revolution/GX/GXMisc.c", cflags=wii_gx_cflags),
                Object(NonMatching, "revolution/GX/GXGeometry.c", cflags=wii_gx_cflags),
                Object(Matching, "revolution/GX/GXFrameBuf.c", cflags=wii_gx_cflags),
                Object(Equivalent, "revolution/GX/gx_light_load.s", add_to_all=False),
                Object(Equivalent, "revolution/GX/GXLight.c", cflags=wii_gx_cflags),
                Object(NonMatching, "revolution/GX/GXTexture.c", cflags=wii_gx_cflags),
                Object(Matching, "revolution/GX/GXBump.c", cflags=wii_gx_cflags),
                Object(Matching, "revolution/GX/GXTev.c", cflags=wii_gx_cflags),
                Object(Matching, "revolution/GX/GXPixel.c", cflags=wii_gx_cflags),
                Object(Matching, "revolution/GX/GXDraw.c", cflags=wii_gx_cflags),
                Object(Matching, "revolution/GX/GXDisplayList.c", cflags=wii_gx_cflags),
                Object(Equivalent, "revolution/GX/gx_transform_ps.s", add_to_all=False),
                Object(Equivalent, "revolution/GX/GXTransform.c", cflags=wii_gx_cflags),
                Object(Matching, "revolution/GX/GXPerf.c", cflags=wii_gx_cflags),
                # GC SDK units paired to free Wii ranges - same DolphinLib profile
                # (GC/1.2.5n + cflags_sdk) the GameCube editions build them with
                Object(Equivalent, "dolphin/os/OSAudioSystem.c", cflags=cflags_sdk, mw_version="GC/1.2.5n"),
                Object(NonMatching, "dolphin/os/OSError.c", cflags=cflags_sdk, mw_version="GC/1.2.5n"),
                Object(Equivalent, "dolphin/gx/GXTev.c", cflags=cflags_sdk_2004, mw_version="GC/1.2.5n"),
                Object(Equivalent, "dolphin/gx/GXPixel.c", cflags=cflags_sdk_2004, mw_version="GC/1.2.5n"),
                Object(Equivalent, "dolphin/gx/GXPerf.c", cflags=cflags_sdk, mw_version="GC/1.2.5n"),
                Object(NonMatching, "dolphin/gx/GXAttr.c", cflags=cflags_sdk_2004, mw_version="GC/1.2.5n"),
                Object(NonMatching, "dolphin/gx/GXInit.c", cflags=cflags_sdk_2004, mw_version="GC/1.2.5n"),
                Object(NonMatching, "dolphin/gx/GXFifo.c", cflags=cflags_sdk_2004, mw_version="GC/1.2.5n"),
                Object(NonMatching, "dolphin/gx/GXMisc.c", cflags=cflags_sdk_2004, mw_version="GC/1.2.5n"),
                Object(NonMatching, "dolphin/gx/GXGeometry.c", cflags=cflags_sdk_2004, mw_version="GC/1.2.5n"),
                Object(NonMatching, "dolphin/gx/GXLight.c", cflags=cflags_sdk_2004, mw_version="GC/1.2.5n"),
                Object(NonMatching, "dolphin/gx/GXTransform.c", cflags=cflags_sdk_2004, mw_version="GC/1.2.5n"),
                Object(NonMatching, "dolphin/gx/GXDraw.c", cflags=cflags_sdk_2004, mw_version="GC/1.2.5n"),
                Object(NonMatching, "dolphin/os/OS.c", cflags=cflags_sdk_2004, mw_version="GC/1.2.5n"),
                Object(NonMatching, "dolphin/os/OSExec.c", cflags=cflags_sdk_2004, mw_version="GC/1.2.5n"),
                Object(NonMatching, "dolphin/os/OSMemory.c", cflags=cflags_sdk, mw_version="GC/1.2.5n"),
                Object(NonMatching, "dolphin/os/OSReset.c", cflags=cflags_sdk_2004, mw_version="GC/1.2.5n"),
            ],
        },
        {
            "lib": "RevolutionSDK",
            "mw_version": "Wii/1.3",
            "cflags": wii_cflags,
            "progress_category": "sdk",
            "objects": [
                # These three retail MEM heap units have no extab/extabindex entries;
                # omitting -Cpp_exceptions off emits those sections even for C sources.
                # Their small static helpers are inlined in retail: without -ipa file
                # MWCC emits out-of-line copies such as FindListContainHeap_,
                # InitMBlock_/InitExpHeap_, and InitFrameHeap_/AllocFromHead_.
                # Finally, retail MEMiInitHeapHead, MEMiFinalizeHeap,
                # AllocUsedBlockFromFreeBlock_, and MEMRecordStateForFrmHeap call the
                # runtime's _savegpr/_restgpr helpers rather than emitting stmw/lmw.
                Object(
                    Matching, "revolution/MEM/mem_heapCommon.c", mw_version="GC/3.0a5.2",
                    extra_cflags=["-Cpp_exceptions off", "-ipa file", "-use_lmw_stmw off"],
                ),
                Object(
                    Matching, "revolution/MEM/mem_expHeap.c", mw_version="GC/3.0a5.2",
                    extra_cflags=["-Cpp_exceptions off", "-ipa file", "-use_lmw_stmw off"],
                ),
                Object(
                    Matching, "revolution/MEM/mem_frameHeap.c", mw_version="GC/3.0a5.2",
                    extra_cflags=["-Cpp_exceptions off", "-ipa file", "-use_lmw_stmw off"],
                ),
                Object(Matching, "revolution/MEM/mem_allocator.c"),
                # Retail mem_list has only the four public list functions; the static
                # SetFirstObject_ helper is inlined into MEMAppendListObject. Without
                # -ipa file MWCC emits it as a fifth out-of-line function, so the whole
                # unit was built with file-level IPA.
                Object(Matching, "revolution/MEM/mem_list.c", mw_version="GC/3.0a5.2", extra_cflags=["-ipa file"]),
                # Retail NANDSecret/NANDCheck contain no extab/extabindex. Without
                # -Cpp_exceptions off MWCC emits those sections and trailing exception
                # code, so the whole C units were built with exceptions off.
                Object(Matching, "revolution/NAND/NANDSecret.c", mw_version="GC/3.0a5.2", extra_cflags=["-Cpp_exceptions off"]),
                Object(Matching, "revolution/NAND/NANDCheck.c", mw_version="GC/3.0a5.2", extra_cflags=["-Cpp_exceptions off"]),
            ],
        },
        {
            # MetroTRK ("TRK_Hollywood_Revolution"). Units whose code is the GameCube's build upstream's
            # file; the others have their own file under src/rvl_trk: the Wii build has no MWTRACE calls,
            # numbers its replies (msghndlr.c, after Petari's), knows no ARAM and no DDH, and has
            # TRK_strlen and OutputData.
            # Compiler: GC/2.6 with the GameCube's flags. GC/2.7, which the other Wii decomps name,
            # gives the same bytes in every unit (main.dol links OK with either); GC/3.0 and the Wii
            # compilers do not (TRKConstructEvent and nearly every other C function differ).
            "lib": "TRK_MINNOW_DOLPHIN",
            "mw_version": "GC/2.6",
            "cflags": cflags_trk,
            "progress_category": "runtime",
            "objects": [
                Object(Matching, "TRK_MINNOW_DOLPHIN/mainloop.c"),
                Object(Matching, "TRK_MINNOW_DOLPHIN/nubevent.c"),
                Object(Matching, "rvl_trk/nubinit.c"),
                Object(Matching, "rvl_trk/msg.c"),
                Object(Matching, "TRK_MINNOW_DOLPHIN/msgbuf.c"),
                # Retail gives serpoll.c and gdev/main.c small-data bss ranges
                # (0x805A26C8-0x805A26D0 and 0x805A26D0-0x805A26D8) beside their
                # large .bss. Their only small objects are the 4-byte globals
                # gTRKInputPendingPtr and gIsInitialized; with the TRK-wide
                # -sdata 0 those would not be small data, so both whole units
                # were built with -sdata 8.
                Object(Matching, "rvl_trk/serpoll.c", extra_cflags=["-sdata 8"]),
                Object(Equivalent, "TRK_MINNOW_DOLPHIN/usr_put.c"),
                Object(Matching, "rvl_trk/dispatch.c"),
                Object(Matching, "rvl_trk/msghndlr.c"),
                Object(NonMatching, "rvl_trk/support.c"),
                Object(Matching, "TRK_MINNOW_DOLPHIN/mutex_TRK.c"),
                Object(Matching, "TRK_MINNOW_DOLPHIN/notify.c"),
                Object(Matching, "TRK_MINNOW_DOLPHIN/flush_cache.c", source="TRK_MINNOW_DOLPHIN/flush_cache.s"),
                Object(Equivalent, "TRK_MINNOW_DOLPHIN/mem_TRK.c"),
                Object(Matching, "TRK_MINNOW_DOLPHIN/__exception.c", source="TRK_MINNOW_DOLPHIN/__exception.s"),
                Object(Matching, "rvl_trk/string_TRK.c"),
                Object(Equivalent, "TRK_MINNOW_DOLPHIN/trk_target_context.s", add_to_all=False),
                Object(Equivalent, "rvl_trk/targimpl.c"),
                # an assembled file in the original: four trap stubs, built from targsupp.s
                Object(Matching, "rvl_trk/targsupp.c", source="rvl_trk/targsupp.s"),
                Object(Matching, "TRK_MINNOW_DOLPHIN/mpc_7xx_603e.c", source="TRK_MINNOW_DOLPHIN/mpc_7xx_603e.s"),
                Object(Equivalent, "TRK_MINNOW_DOLPHIN/trk_boot.s", add_to_all=False),
                Object(Equivalent, "rvl_trk/dolphin_trk.c"),
                Object(Matching, "rvl_trk/main_TRK.c"),
                Object(Equivalent, "TRK_MINNOW_DOLPHIN/trk_context_load.s", add_to_all=False),
                Object(Equivalent, "rvl_trk/dolphin_trk_glue.c"),
                Object(Matching, "TRK_MINNOW_DOLPHIN/targcont.c"),
                Object(Matching, "TRK_MINNOW_DOLPHIN/target_options.c"),
                Object(Matching, "TRK_MINNOW_DOLPHIN/mslsupp.c"),
                Object(Matching, "TRK_MINNOW_DOLPHIN/UDP_Stubs.c"),
                Object(Matching, "TRK_MINNOW_DOLPHIN/CircleBuffer.c"),
                Object(Matching, "rvl_trk/gdev/main.c", extra_cflags=["-sdata 8"]),
                Object(Matching, "TRK_MINNOW_DOLPHIN/MWCriticalSection_gc.cpp"),
            ],
        },
        {
            "lib": "Runtime.PPCEABI.H",
            "mw_version": "Wii/1.3",
            "cflags": wii_cflags,
            "progress_category": "runtime",
            "objects": [
                # per-unit mw_version/extra_cflags verified by clean-rebuild tests:
                # __va_arg, NMWException, GCN_mem_alloc and Gecko_ExceptionPPC drop to
                # 92-99% under the library default (Wii/1.3)
                Object(Matching, "rvl_runtime/__mem.c", extra_cflags=["-Cpp_exceptions off"]),
                Object(Matching, "Runtime.PPCEABI.H/__va_arg.c", mw_version="GC/3.0a5.2", extra_cflags=["-Cpp_exceptions off"]),
                # the Wii runtime sources differ from upstream's and live under src/rvl_*
                Object(Matching, "rvl_runtime/global_destructor_chain.c", extra_cflags=["-Cpp_exceptions off"]),
                Object(Matching, "Runtime.PPCEABI.H/NMWException.cp", mw_version="GC/3.0a5.2", extra_cflags=["-Cpp_exceptions on"]),
                Object(Equivalent, "Runtime.PPCEABI.H/ptmf.c"),
                Object(Matching, "Runtime.PPCEABI.H/ptmf_call.s", add_to_all=False),
                Object(Matching, "rvl_runtime/runtime.s"),
                Object(Equivalent, "rvl_runtime/init_cpp_exceptions.s", add_to_all=False),
                # Retail has no extab/extabindex in this unit. Rebuilding without
                # -Cpp_exceptions off emits both sections even though the code itself
                # is unchanged, so the whole unit was built with exceptions off.
                Object(Equivalent, "rvl_runtime/__init_cpp_exceptions.cpp", extra_cflags=["-Cpp_exceptions off"]),
                # ExPPC_UnwindStack stays at 97.3 %: retail keeps the 0 it stores into info->action_pointer in
                # r31, we get r25 and every other register one lower. Tried without effect: GC/3.0a3 to
                # 3.0a5.2 and Wii/1.0 to 1.3, -ipa file, -inline on/off/deferred/smart, -O4,s, -O3, the
                # single -opt switches, -RTTI off, pooled strings. xenoblade has the same source text.
                Object(NonMatching, "Runtime.PPCEABI.H/Gecko_ExceptionPPC.cp", mw_version="GC/3.0a5.2", extra_cflags=["-Cpp_exceptions on", "-RTTI on", "-str reuse,readonly"]),
                Object(Matching, "Runtime.PPCEABI.H/GCN_mem_alloc.c", mw_version="GC/3.0a5.2", extra_cflags=["-Cpp_exceptions off", "-rostr", "-str reuse"]),
            ],
        },
        {
            "lib": "MSL_C.PPCEABI.bare.H",
            "mw_version": "Wii/1.3",
            # the Wii MSL differs from upstream's: its headers live under include/rvl_msl
            # and shadow the shared include paths (math_api.h declares __fpclassifyd extern)
            "cflags": ["-i include/rvl_msl", *wii_cflags],
            "progress_category": "runtime",
            "objects": [
                # per-unit flags verified by clean-rebuild tests: the Double_precision
                # units most need -fp_contract off (fuzzy drops to 60-94% otherwise); s_tan
                # is the exception that matches under the library default.
                # Their compiler is GC/3.0a5 (GC/3.0a3 gives the same code): under GC/3.0a5.2
                # __ieee754_asin, __kernel_tan and atan lose the dead store of p/q that retail
                # keeps (98.7, 98.7, 97.2 %) and __ieee754_pow falls from 99.8 to 94.3 %, while
                # every other unit of the directory is the same under both. printf.c is the
                # other way round (double2hex 94.4 %, __pformatter 92.8 % under GC/3.0a5).
                # The files under src/rvl_msl are the Wii's MSL, after xenoblade's (2009), with printf.c and
                # signal.c after Petari's (2007): our printf still has the four-argument __pformatter.
                # Compiler GC/3.0a5.2 with -ipa file, -inline on, -fp_contract off, pooled strings and
                # exceptions on (every file with a non-leaf function has extab entries in retail).
                # Evidence: with exactly these flags alloc.c, ansi_fp.c, file_io.c, mbstring.c, strtoul.c
                # and the rest come out byte for byte without a source change; Wii/1.1 and Wii/1.3 lose
                # __num2dec, __two_exp, __fwrite, _fseek and about thirty more functions.
                *[
                    Object(status, f"rvl_msl/{name}", mw_version="GC/3.0a5.2", cflags=wii_msl_cflags)
                    for status, name in [
                        (Matching, "alloc.c"),
                        (Matching, "ansi_files.c"),
                        (Matching, "ansi_fp.c"),
                        (Matching, "arith.c"),
                        (Matching, "direct_io.c"),
                        (Matching, "file_io.c"),
                        (Matching, "FILE_POS.c"),
                        (Matching, "mbstring.c"),
                        (Matching, "math_api.c"),
                        (Matching, "printf.c"),
                        (Matching, "scanf.c"),
                        (Matching, "signal.c"),
                        (Matching, "string.c"),
                        (Matching, "strtoul.c"),
                        (Matching, "wstring.c"),
                        (Matching, "wchar_io.c"),
                        (Matching, "abort_exit_ppc_eabi.c"),
                        (Matching, "math_sun.c"),
                        (Matching, "extras.c"),
                    ]
                ],
                # the math files among them: GC/3.0a5 like the rest of Double_precision (see below), no extab
                *[
                    Object(status, f"rvl_msl/{name}", mw_version="GC/3.0a5", cflags=wii_msl_cflags, extra_cflags=["-Cpp_exceptions off"])
                    for status, name in [
                        (Matching, "e_log10.c"),
                        (Matching, "e_pow.c"),
                        (Matching, "e_rem_pio2.c"),
                        (Matching, "fminmaxdim.c"),
                        (Matching, "s_ceil.c"),
                        (Matching, "w_log10.c"),
                    ]
                ],
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common/errno.c"),
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common/float.c"),
                # the Wii MSL sources differ from upstream's and live under src/rvl_msl
                Object(Matching, "rvl_msl/mem.c", mw_version="GC/3.0a5.2"),
                # GC/2.6: __copy_longs_* only match with the older compiler (89-91% under Wii/1.3)
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common/mem_funcs.c", mw_version="GC/2.6"),
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common/rand.c"),
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/e_acos.c", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions off", "-fp_contract off"]),
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/e_asin.c", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions off", "-fp_contract off"]),
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/e_atan2.c", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions off", "-fp_contract off"]),
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/e_log.c", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions off", "-fp_contract off"]),
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/e_sqrt.c", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions off", "-fp_contract off"]),
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/k_cos.c", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions off", "-fp_contract off"]),
                Object(Matching, "rvl_msl/k_rem_pio2.c", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions off", "-fp_contract off"]),
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/k_sin.c", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions off", "-fp_contract off"]),
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/k_tan.c", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions off", "-fp_contract off"]),
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/s_atan.c", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions off", "-fp_contract off"]),
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/s_copysign.c", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions off"]),
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/s_cos.c", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions off"]),
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/s_floor.c", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions off", "-fp_contract off"]),
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/s_frexp.c", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions off", "-fp_contract off"]),
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/s_ldexp.c", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions off", "-fp_contract off"]),
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/s_modf.c", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions off", "-fp_contract off"]),
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/s_sin.c", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions off"]),
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/s_tan.c", extra_cflags=["-Cpp_exceptions off"]),
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/w_acos.c", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions off", "-fp_contract off"]),
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/w_asin.c", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions off", "-fp_contract off"]),
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/w_atan2.c", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions off", "-fp_contract off"]),
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/w_fmod.c", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions off", "-fp_contract off"]),
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/w_pow.c", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions off", "-fp_contract off"]),
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/w_sqrt.c", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions off", "-fp_contract off"]),
                # GC MSL units paired to free Wii ranges
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common/buffer_io.c"),
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common/misc_io.c"),
                Object(Matching, "MSL_C.PPCEABI.bare.H/PPC_EABI/uart_console_io_gcn.c", mw_version="GC/2.6"),
                Object(Matching, "MSL_C.PPCEABI.bare.H/MSL_Common_Embedded/Math/Double_precision/e_fmod.c", mw_version="GC/3.0a5", extra_cflags=["-Cpp_exceptions off", "-fp_contract off"]),
            ],
        },
    ]

    # Retail's engine units sit in one block, 0x802A5358-0x802C19B8 (ut.cpp to fbeffect.cpp, with
    # the shared game/ctoon.cpp at 0x802BD294; its one function builds the same either way), and
    # that block is built without auto-inlining: every unit in it scores the same or higher under
    # -inline noauto than without it (cgs 53 -> 89 %, cfont 56 -> 100 %, cobj 68 -> 100 %), the ones
    # with no auto-inline candidates (cobjvec, cresfile, ctrigon) included, while units outside it,
    # such as params.cpp (100 -> 78 %) and sound.cpp (99 -> 44 %), lose. So the flag belongs to the
    # block as a whole, the way a library built from its own makefile carries its own flags.
    # Other decomps set -inline noauto for a whole library the same way:
    # zeldaret/tp's framework library, https://github.com/zeldaret/tp/blob/c8fa8c9e2aab72cf4e5db0e5d1c84a9ea6ee6eb0/configure.py#L415-L422
    # pikmin2's JAudio_DSP, https://github.com/projectPiki/pikmin2/blob/999140f3521e22619547d5d61862a6577ea73882/configure.py#L632
    # Petari's JASDsp, https://github.com/SMGCommunity/Petari/blob/b135d455de9bb5ed32ceaa80ee49dfcbed0526c0/configure.py#L328
    WII_NOAUTO_BLOCK = {
        "game/wii/ut.cpp", "game/wii/mem.cpp", "game/wii/cobjvec.cpp", "game/wii/cgs.cpp", "game/wii/cgs_nw.cpp",
        "game/wii/cprimitive.cpp", "game/wii/cpad.cpp", "game/wii/cfont.cpp", "game/wii/cfile.cpp",
        "game/wii/ctpl.cpp", "game/wii/cresfile.cpp", "game/wii/cobj.cpp", "game/wii/ctrigon.cpp",
        "game/ctoon.cpp", "game/wii/ceffect.cpp", "game/wii/fbeffect.cpp",
    }
    for obj in (o for lib in config.libs for o in lib["objects"] if o.name in WII_NOAUTO_BLOCK):
        obj.options["extra_cflags"] = [*obj.options["extra_cflags"], "-inline noauto"]

# Optional callback to adjust link order. This can be used to add, remove, or reorder objects.
# This is called once per module, with the module ID and the current link order.
#
# For example, this adds "dummy.c" to the end of the DOL link order if configured with --non-matching.
# "dummy.c" *must* be configured as a Matching (or Equivalent) object in order to be linked.
def link_order_callback(module_id: int, objects: List[str]) -> List[str]:
    if config.non_matching and module_id == 0:
        # Source-built constructors and destructors emit these ABI symbols.
        # CGs owns the primitive members and emits their weak destructors.
        # The source-built OSMemory.c also defines the protection initializer.
        extracted_only = {
            "game/title_cxobj_vtables.cpp",
            "game/drsmenu_vtable.cpp",
            "game/atcmenu_vtables.cpp",
            "game/cinterpolationeaseout_rtti.cpp",
            "game/cjointrot_vtable.cpp",
            "game/params_vtable.cpp",
            "game/xpad_vtable.cpp",
            "game/memcardscan_format.cpp",
            "game/memcardscan_buffer.cpp",
            "game/memcardpreview_buffers.cpp",
            "game/ccameraactionarea_vtable.cpp",
            "game/ccameraactionmap_vtable.cpp",
            "game/ccameraactionreset_vtable.cpp",
            "game/cjarchive_rtti.cpp",
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
    category
    for category in (
        ProgressCategory("game", "Game Code"),
        ProgressCategory("sdk", "SDK Code"),
        ProgressCategory("nw4r", "NW4R Code"),
        ProgressCategory("hsd", "HSD Code"),
        ProgressCategory("runtime", "Runtime Code"),
        ProgressCategory("rels", "REL Code"),
    )
    if any(lib.get("progress_category") == category.id for lib in config.libs)
]
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



def add_text_step(config: ProjectConfig) -> None:
    # The game's displayed text is not in the repository: tools/gentext.py reads it
    # from the disc's main.dol into the .inc files src/game/wii/stage_data.cpp includes.
    # restat: an unchanged .inc keeps its time, so nothing recompiles.
    from tools.gentext import table_outputs

    outputs = table_outputs(config.version, Path("."))
    if not outputs:
        return
    config.custom_build_rules = (config.custom_build_rules or []) + [
        {
            "name": "gentext",
            "command": f"$python tools/gentext.py {config.version}",
            "description": "GENTEXT $out",
            "restat": True,
        }
    ]
    steps = config.custom_build_steps or {}
    steps.setdefault("pre-compile", []).append(
        {
            "rule": "gentext",
            "outputs": outputs,
            "inputs": [
                Path("orig") / config.version / "sys" / "main.dol",
                Path("config") / config.version / "symbols.txt",
                Path("tools/gentext.py"),
            ],
        }
    )
    config.custom_build_steps = steps

if args.mode == "configure":
    add_text_step(config)
    # Write build.ninja and objdiff.json
    generate_build(config)
    append_treemap_step(config)
elif args.mode == "progress":
    # Print progress information
    calculate_progress(config)
else:
    sys.exit("Unknown mode: " + args.mode)
