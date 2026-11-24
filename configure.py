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
    "DEBUG",       # 0
    "RELEASE",     # 1
    "MASTERDEBUG", # 2
    "MASTER",      # 3
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
config.generate_map = args.map
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
config.dtk_tag = "v1.8.3"
config.objdiff_tag = "v3.6.1"
config.sjiswrap_tag = "v1.2.2"
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
    "-warn off",
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

# xParSys includes "xPTankPool.h", zToss.h "zlightweight.h": __FILE__ needs each exact file. Made on Linux with any
# wrapper: wibo resolves file names case-insensitively, so without them __FILE__ is the file's own name
if sys.platform == "linux":
    for alias, target in (
        ("src/rt/Engine/Core/x/xPTankPool.h", "xPtankPool.h"),
        ("src/rt/Engine/Game/zlightweight.h", "zLightWeight.h"),
    ):
        alias = Path(alias)
        if not alias.is_symlink():
            alias.symlink_to(target)

# Base flags, common to most GC/Wii games.
# Generally leave untouched, with overrides added below.
cflags_base = [
    "-nodefaults",
    "-proc gekko",
    "-align powerpc",
    "-enum int",
    "-fp hardware",
    "-Cpp_exceptions off",
    # "-W all",
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
    f"-i build/{config.version}/include",
    "-i src/PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Include",
    #"-i src/PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Include",
    #"-i src/PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Include",
    "-i src/PowerPC_EABI_Support/MSL/MSL_C/PPC_EABI/Include",
    "-i src/PowerPC_EABI_Support/MSL/MSL_C++/MSL_Common/Include",
]

# Debug flags
if version_num == 0:
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

# Metrowerks library flags
cflags_runtime = [
    *cflags_base,
    "-use_lmw_stmw on",
    "-str reuse,pool,readonly",
    "-gccinc",
    "-common off",
    "-inline auto",
    "-O4,p",
]

# Dolphin SDK flags (dolsdk2004's Makefile). D links the debug libraries, R/MD/M the release ones.
cflags_sdk_base = [
    "-char unsigned",
    "-nodefaults",
    "-proc gekko",
    "-fp hard",
    "-Cpp_exceptions off",
    "-enum int",
    "-warn pragmas",
    "-requireprotos",
    '-pragma "cats off"',
    "-D__GEKKO__",
    "-nosyspath",
    "-i src/dolphin/include",
    "-i src/dolphin/include/libc",
    "-ir src/dolphin/src",
]
cflags_sdk_debug = [
    "-opt level=0",
    "-inline off",
    "-schedule off",
    "-sym on",
    *cflags_sdk_base,
    "-DDEBUG",
]
cflags_sdk_release = [
    "-O4,p",
    "-inline auto",
    "-sym on",
    *cflags_sdk_base,
    "-DRELEASE",
]
cflags_sdk = cflags_sdk_debug if version_num == 0 else cflags_sdk_release
sdk_char_signed = ["-char signed"]

# SDK_REVISION: each library's build date (YYYYMMDD) from its version string; libraries without one get the date
# most libraries carry. OS and GX are the Nov 10 2004 patch in D/MD/M, the unpatched Apr 5 2004 ones in R.
sdk_dates = {
    "gx": [20041110, 20040405, 20041110, 20041110][version_num],
    "os": [20041110, 20040405, 20041110, 20041110][version_num],
    "vi": 20040407,
}
sdk_date_default = 20040405


def sdk_revision(lib_name: str) -> str:
    return f"-DSDK_REVISION={sdk_dates.get(lib_name, sdk_date_default)}"


# EXIBios.c is built at -O3,p instead of -O4,p in the release library
cflags_sdk_exibios = [("-O3,p" if f == "-O4,p" else f) for f in cflags_sdk] + [sdk_revision("exi")]

# REL flags
cflags_rel = [
    *cflags_base,
    "-sdata 0",
    "-sdata2 0",
    "-O4,p",
]

# RenderWare flags
cflags_rwcore_base = [
    *cflags_base,
    "-sym on",
    "-DDOLPHIN",
    "-DRW_USE_SPF",
    "-DRWNOVECMULTFUNCS",
    "-gccinc",
    "-i src/dolphin/include",
    "-i src/rwsdk/include/gcn",
    "-i src/rwsdk/driver/common",
    "-i src/rwsdk/driver/gcn",
    "-i src/rwsdk/os/gcn",
    "-i src/rwsdk/src",
    "-i src/rwsdk/src/pipe/p2",
    "-i src/rwsdk/src/pipe/p2/gcn",
    "-i src/rwsdk/src/plcore",
]
cflags_rwcore_debug = [
    *cflags_rwcore_base,
    "-DRWDEBUG",
    "-inline off",
]
cflags_rwcore_release = [
    *cflags_rwcore_base,
    "-O4,p",
]
cflags_rwcore_list = [
    cflags_rwcore_debug,
    cflags_rwcore_release,
    cflags_rwcore_release,
    cflags_rwcore_release,
]
cflags_rwcore = cflags_rwcore_list[version_num]

cflags_rpcollis = [
    "-i src/rwsdk/plugin/collis",
    *cflags_rwcore,
]
cflags_rphanim = [
    "-i src/rwsdk/plugin/hanim",
    *cflags_rwcore,
]
cflags_rpmatfx = [
    "-i src/rwsdk/plugin/matfx",
    "-i src/rwsdk/plugin/matfx/gcn",
    *cflags_rwcore,
]
cflags_rpptank = [
    "-i src/rwsdk/plugin/ptank",
    "-i src/rwsdk/plugin/ptank/gcn",
    "-i src/rwsdk/plugin/ptank/generic",
    *cflags_rwcore,
]
cflags_rpskinmatfx = [
    "-i src/rwsdk/plugin/skin2",
    "-i src/rwsdk/plugin/skin2/gcn",
    *cflags_rwcore,
]
cflags_rpusrdat = [
    "-i src/rwsdk/plugin/userdata",
    *cflags_rwcore,
]
cflags_rpworld = [
    "-i src/rwsdk/world",
    "-i src/rwsdk/world/pipe",
    "-i src/rwsdk/world/pipe/p2",
    "-i src/rwsdk/world/pipe/p2/gcn",
    *cflags_rwcore,
]
cflags_rtanim = [
    "-i src/rwsdk/tool/anim",
    *cflags_rwcore,
]
cflags_rtcharse = [
    "-i src/rwsdk/tool/charse",
    *cflags_rwcore,
]
cflags_rtfsyst = [
    "-i src/rwsdk/tool/fsyst",
    "-i src/rwsdk/tool/fsyst/gcn",
    *cflags_rwcore,
]
cflags_rtintsec = [
    "-i src/rwsdk/tool/intsec",
    *cflags_rwcore,
]
cflags_rtquat = [
    "-i src/rwsdk/tool/quat",
    *cflags_rwcore,
]
cflags_rtskinsp = [
    "-i src/rwsdk/tool/sknsplit",
    *cflags_rwcore,
]
cflags_rtslerp = [
    "-i src/rwsdk/tool/slerp",
    *cflags_rwcore,
]
cflags_rttilerd = [
    "-i src/rwsdk/tool/tilerd",
    *cflags_rwcore,
]

# Game-specific flags
cflags_rat_base = [
    *cflags_base,
    "-lang=c++",
    "-pool off",
    "-schedule on",
    "-char signed",
    "-str reuse,pool,readonly",
    "-fp fmadd",
    "-use_lmw_stmw on",
    '-pragma "cpp_extensions on"',
    "-sym on",
    "-inline off",
    "-i src/rt",
    "-i src/rt/Engine/Core/gc",
    "-i src/rt/Engine/Core/x",
    "-i src/rt/Engine/Game",
    "-i src/rwsdk/include",
    "-i src/dolphin/include",
    "-i src/revolution/include",
    "-i src/bink/bink",
    "-i src/fmod/include",
    "-DDEBUG=0",
    "-DRELEASE=0",
    "-DMASTERDEBUG=0",
    "-DMASTER=0",
    "-DGRC=0",
    "-prefix prefix.mch",
]
cflags_rat_d = [
    *cflags_rat_base,
    "-DDEBUG=1",
    r'-DBUILDNUM="\"129\""',
    r'-D__DATE__="\"Jan 11 2006\""',
    r'-D__TIME__="\"00:29:14\""',
    "-DRWDEBUG",
    "-opt peep, speed",
    "-sdata 20",
    "-sdata2 20",
]
cflags_rat_r = [
    *cflags_rat_base,
    "-DRELEASE=1",
    "-DGRC=1",
    r'-DBUILDNUM="\"135\""',
    r'-D__DATE__="\"Jan 18 2006\""',
    r'-D__TIME__="\"15:52:40\""',
    "-opt level=4, peep, speed",
    "-sdata 24",
    "-sdata2 24",
]
cflags_rat_md = [
    *cflags_rat_base,
    "-DMASTERDEBUG=1",
    "-opt level=4, peep, space",
    "-sdata 64",
    "-sdata2 64",
]
cflags_rat_m = [
    *cflags_rat_base,
    "-DMASTER=1",
    "-opt level=4, peep, space",
    "-sdata 64",
    "-sdata2 64",
    "-sym off",
]
cflags_rat_list = [
    cflags_rat_d,
    cflags_rat_r,
    cflags_rat_md,
    cflags_rat_m,
]
cflags_rat = cflags_rat_list[version_num]
config.linker_version = "GC/2.6"

# Game code prefix: include/prefix.pch++, precompiled per build with the game flags into
# build/<version>/include/prefix.mch (found through -i build/<version>/include) and passed to
# every game unit with -prefix (cflags_rat_base).
config.precompiled_headers = [
    {
        "source": "prefix.pch++",
        "mw_version": config.linker_version,
        "cflags": [f for f in cflags_rat if f != "-prefix prefix.mch"],
    }
]
# decomp.me contexts read the prefix header itself (scratches can't use the .mch).
config.context_prefixes = {"prefix.mch": Path("include/prefix.pch++")}


# Helper function for Dolphin libraries
def DolphinLib(lib_name: str, objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "mw_version": "GC/1.2.5n",
        "cflags": [*cflags_sdk, sdk_revision(lib_name)],
        "progress_category": "sdk",
        "objects": objects,
    }

# Helper function for RenderWare libraries
def RenderWareLib(lib_name: str, cflags: List[str], objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "mw_version": "GC/2.5",
        "cflags": cflags,
        "progress_category": "sdk",
        "objects": objects,
    }

# Helper function for REL script objects
def Rel(lib_name: str, objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "mw_version": "GC/1.3.2",
        "cflags": cflags_rel,
        "progress_category": "game",
        "objects": objects,
    }


Matching = True                   # Object matches and should be linked
NonMatching = False               # Object does not match and should not be linked
Equivalent = config.non_matching  # Object should be linked when configured with --non-matching


# Object is only matching for specific versions
def MatchingFor(*versions):
    return config.version in versions


# A game object; keyword flags become compiler flags (sym="on" -> -sym on)
def GameObject(completed: bool, name: str, **flags: Any) -> Object:
    return Object(completed, name, extra_cflags=[f"-{flag} {value}" for flag, value in flags.items()])


config.warn_missing_config = True
config.warn_missing_source = False
config.libs = [
    {
        "lib": "RT",
        "mw_version": config.linker_version,
        "cflags": cflags_rat,
        "progress_category": "game",
        "objects": [
            GameObject(Matching, "rt/Engine/Core/x/xAnim.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xBase.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xbinio.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xBound.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xCam.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xCamFollow.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xCamSupport.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xClimate.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xClumpColl.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xCM.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xCollide.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xCollideFast.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xColor.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xCounter.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xCutscene.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xDebug.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xDebugGraph.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xDebugTweak.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xDecal.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xDraw.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xEnt.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xEntBoulder.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xEntDrive.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xEntMotion.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xEnv.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xEvent.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xFactory.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xFFX.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xFilter.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xFMV.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xFog.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xFont.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xFX.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xGrid.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xGroup.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xHierarchyBound.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xhipio.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xHud.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xHudFontMeter.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xHudMeter.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xHudModel.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xHudUnitMeter.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xIni.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xJaw.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xJSP.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xLight.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xLightKit.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xMath.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xMath2.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xMath3.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xMemMgr.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xModel.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xModelBucket.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xMorph.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xMovePoint.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xNavigationMeshAsset.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xordarray.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xOutputMgr.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xPad.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xPar.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xParCmd.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xParEmitter.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xParEmitterType.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xParGroup.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xParMgr.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xParSys.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xPartition.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xpkrsvc.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xPtankPool.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xQuickCull.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xRand.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xRegionSupport.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xRenderState.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xRMemData.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xRumbleEmitter.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xRumbleManager.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xsavegame.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xScene.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xScreenWarp.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xScrFx.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xserializer.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xShadow.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xShadowSimple.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xSkyDome.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xSndMgr.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xSoundFX.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xSpacePartitionTree.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xSpline.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xSplineAsset.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xstransvc.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xString.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xSubTitles.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xSurface.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xTextAsset.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xTextureManager.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xTimer.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xTimestamp.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xTransient.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xTRC.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xUpdateCull.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xutil.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xVec3.cpp"),
            GameObject(Matching, "rt/Engine/Core/x/xVolume.cpp"),
            GameObject(Matching, "rt/Engine/Core/gc/iAnimSKB.cpp"),
            GameObject(Matching, "rt/Engine/Core/gc/iAsync.cpp"),
            GameObject(Matching, "rt/Engine/Core/gc/iCutscene.cpp"),
            GameObject(Matching, "rt/Engine/Core/gc/iDebug.cpp"),
            GameObject(Matching, "rt/Engine/Core/gc/iDraw.cpp"),
            GameObject(Matching, "rt/Engine/Core/gc/iEnv.cpp"),
            GameObject(Matching, "rt/Engine/Core/gc/iFile.cpp"),
            GameObject(Matching, "rt/Engine/Core/gc/iFMV.cpp"),
            GameObject(Matching, "rt/Engine/Core/gc/iFX.cpp"),
            GameObject(Matching, "rt/Engine/Core/gc/iLight.cpp"),
            GameObject(Matching, "rt/Engine/Core/gc/iMath.cpp"),
            GameObject(Matching, "rt/Engine/Core/gc/iMath3.cpp"),
            GameObject(Matching, "rt/Engine/Core/gc/iMemMgr.cpp"),
            GameObject(Matching, "rt/Engine/Core/gc/iModel.cpp"),
            GameObject(Matching, "rt/Engine/Core/gc/iMorph.cpp"),
            GameObject(Matching, "rt/Engine/Core/gc/iPad.cpp"),
            GameObject(Matching, "rt/Engine/Core/gc/iParMgr.cpp"),
            GameObject(Matching, "rt/Engine/Core/gc/isavegame.cpp"),
            GameObject(Matching, "rt/Engine/Core/gc/iScrFX.cpp"),
            GameObject(Matching, "rt/Engine/Core/gc/iSnd.cpp"),
            GameObject(Matching, "rt/Engine/Core/gc/iSystem.cpp"),
            GameObject(Matching, "rt/Engine/Core/gc/iTime.cpp"),
            GameObject(Matching, "rt/Engine/Core/gc/iTRC.cpp"),
            GameObject(Matching, "rt/Engine/Core/gc/iAnim.cpp"),
            GameObject(Matching, "rt/Engine/Game/zActionLine.cpp"),
            GameObject(Matching, "rt/Engine/Game/zAnimCache.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zAnimList.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zAssetTypes.cpp"),
            GameObject(Matching, "rt/Engine/Game/zBase.cpp"),
            GameObject(Matching, "rt/Engine/Game/zCheat.cpp"),
            GameObject(Matching, "rt/Engine/Game/zCollGeom.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zCommonPlayer.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zConditional.cpp"),
            GameObject(Matching, "rt/Engine/Game/zConfiguration.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zCutsceneMgr.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zDestructible.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zDispatcher.cpp"),
            GameObject(Matching, "rt/Engine/Game/zEditorLink.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zEGenerator.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zEnt.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zEntButton.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zEntSimpleObj.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zEntTrigger.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zEnv.cpp"),
            GameObject(Matching, "rt/Engine/Game/zEvent.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zExplosion.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zFire.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zFlameEmitter.cpp"),
            GameObject(Matching, "rt/Engine/Game/zFMV.cpp"),
            GameObject(Matching, "rt/Engine/Game/zFX.cpp"),
            GameObject(Matching, "rt/Engine/Game/zFXRibbonPool.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zGame.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zGameExtras.cpp"),
            GameObject(Matching, "rt/Engine/Game/zGameState.cpp"),
            GameObject(Matching, "rt/Engine/Game/zGrapple.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zGrid.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zHangable.cpp"),
            GameObject(Matching, "rt/Engine/Game/zHitParameters.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zLaserBeam.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zLightning.cpp"),
            GameObject(Matching, "rt/Engine/Game/zLightWeight.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zLOD.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zMain.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zMenu.cpp"),
            GameObject(Matching, "rt/Engine/Game/zMovePoint.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zMusic.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zNPCMgr.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zNPCNavigation.cpp"),
            GameObject(Matching, "rt/Engine/Game/zNPCNavigationMesh.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zParCmd.cpp"),
            GameObject(Matching, "rt/Engine/Game/zParEmitter.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zPendulum.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zPlatform.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zPlayer.cpp"),
            GameObject(Matching, "rt/Engine/Game/zPlayerBase.cpp"),
            GameObject(Matching, "rt/Engine/Game/zPortal.cpp"),
            GameObject(Matching, "rt/Engine/Game/zReference.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zRenderState.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zSaveLoad.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zScene.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zScript.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zShrapnel.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zSurface.cpp"),
            GameObject(Matching, "rt/Engine/Game/zSymbol.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zTalkBox.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zTaskBox.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zTextBox.cpp"),
            GameObject(Matching, "rt/Engine/Game/zTimer.cpp"),
            GameObject(Matching, "rt/Engine/Game/zTrainCar.cpp"),
            GameObject(Matching, "rt/Engine/Game/zTransient.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zUI.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zUIBox.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zUIImage.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zUIMgr.cpp"),
            GameObject(Matching, "rt/Engine/Game/zUIModel.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zUIText.cpp"),
            GameObject(Matching, "rt/Engine/Game/zUIUserString.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zVar.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zVolume.cpp"),
            GameObject(NonMatching, "rt/Engine/Game/zWaterhose.cpp"),
            GameObject(Matching, "rt/Engine/Game/zZipLine.cpp"),
            GameObject(Matching, "rt/Engine/Core/gc/screenshot.c", sym="off"),
            GameObject(Matching, "rt/Engine/Core/x/xCollideSpherePolygon.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zAsync.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/x/xParticleBatch.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zSmoke.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zSplash.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zIncrediSlam.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/x/xScrPulseGlare.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zBehavior.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zWater.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zTimeHandler.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/x/xCollideSweptSphere.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/x/xScreenFade.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/x/xCollideSphereBox.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zReticle.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zGrass.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/x/xFXCameraTexture.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/x/xFXHighDynamicRange.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/x/xCollideSpherePolygonUnprocessed.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zInteraction.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zBomb.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zProgressScript.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/x/xHudText.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zFlameThrower.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zRocket.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/x/xHudImage.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zReactiveAnimation.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zDust.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/x/xProjectedTexture.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zSpotlight.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zPickup.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNPCGroup.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zThrowableSystem.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zUIController.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/x/xLightVolume.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zUICustom.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zUIIncredibles.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/x/xMemoryManager.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/x/xMemoryManagerFixed.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/x/xMemoryManagerGeneral.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zParticleGenerator.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zParticleLocator.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zParticleSystems.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zParticleSystemWaterfall.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zUberLaser.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zLoadingScreen.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/x/xModelBlur.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/gc/iWater.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/gc/water/fxembm.c", sym="on"),
            GameObject(Matching, "rt/Engine/Core/gc/water/fxmain.c", sym="on"),
            GameObject(Matching, "rt/Engine/Core/gc/water/texcache.c", sym="on"),
            GameObject(Matching, "rt/Engine/Core/gc/water/texconv.c", sym="on"),
            GameObject(Matching, "rt/Engine/Core/gc/water/textures.c", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zEnergyStream.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zWireFrameAsset.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zJSPExtraData.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zRubbleGenerator.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zParticleFXCommon.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/x/containers.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/x/xFunctionGenerator.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/x/xModelWarp.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zCombo.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zFrozoneIce.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/x/xRenderBuffer.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zFXFireTrail.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zFXEnergySystem.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zUICustomSysMessage.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/gc/iARAMTmp.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zSparkEmitter.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zToss.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/x/xIM.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zParticleCustom.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zShrapnelEmitter.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zSmashRing.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zTransientModel.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/x/iCamera.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zNPCAsset.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zFXLightDust.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNPCBehavior.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zPad.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNPCDirectMode.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/gc/i2dQuadBuffer.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zMeshNavigation.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zMeshCollision.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zMeshPosition.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/x/xLightEffect.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zShowerSystem.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zNPCPerception.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zPlayerInput.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zPlayerConstrainer.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNPCSquad.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCommonPlayerActions.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zPlayerAction.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zAnimCustom.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/x/xCamTransition.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zFrostEffect.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNPCCustomAV.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zFrozoneBlast.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zParticleSystemSnow.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zFXGlint.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zParticleSystemRain.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zNPCCoverPoint.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zVent.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zVentType.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNPCDodgeList.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zPlayerContainer.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zFXGoo.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zGoo.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zScreenWarp.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zFXPunchSnow.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zFXPunchStars.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zUICustomJoinScreenManager.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zNPCMove.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zFXShockwave.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zParSysSnowsplosion.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zParticleSystemEarthDust.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zFXScreenDecal.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zUICustomMissionSelect.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/gc/iDebugLink.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/x/xDebugLink.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNavigationMesh.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zParticleSystemPop.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/x/xOneLiner.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Core/x/xSpringy.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zConversation.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zParticleSystemBurrowKickup.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zUICustomVolume.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zOneLinerSystem.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zUIUVs.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCam2Player.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zUICustomStatsScreen.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zForceFieldEffect.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCheckpoint.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNPCLevelSpecific.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zUICustomCheckpoints.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zUICustomControlOverlay.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zUICustomLevelManager.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zUICustomPauseScreenManager.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zUICustomPowerEffect.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zUICustomSceneImage.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zUICustomScrollers.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zUICustomSecretsScreenControl.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zUICustomSoundMode.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zUICustomStatic.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zUICustomUpgrade.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zUICustomWireframe.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zUICustomArtworkManager.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNPCFootsteps.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zPlayerOneLinerData.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zEconomics.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zRemyPlayer.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zRemyPlayerActions.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCam.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCameraCurve.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCameraFly.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zCameraRBand.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCameraTweak.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCamMarker.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCamPlayer.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zCamPool.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCamPreset.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCamSpline.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zCamSplineCommonMix.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zCamTargetSpline.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zSlideCamera.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNPC.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zNPCCommon.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zNPCCommonBehavior.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zNPCTypes.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zNPCProjectileFrozen.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zNPCProjectileGrub.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zNPCProjectileGrubFX.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zNPCProjectileMetalSheet.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zNPCProjectileTrackingGrub.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCombat.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zCombatAttack.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCombatEffects.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zCombatSystem.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCineFX.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCineFXShared.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zCineFXTableSceneUT11.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zCineFXTableTest.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zHud.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zContextObject.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zContextObjectTightrope.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zContextObjectPoleSwing.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCommonPlayerContextActions.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zSpringboard.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zCarryableObject.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zContextObjectClimbableRope.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zPlayerActionRopeClimb.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zUICustomControllerManager.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zUICustomSaveLoad.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zParticleSystemPuffer.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNPCOneLiner.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zUICustomConditionallyDisabled.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zUICustomMissionCompleteManager.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zUICustomOptionsManager.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zPlayerActionWallRun.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zPlayerActionWallClimb.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zHeistManager.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zHeistLookoutPoint.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zHeistMovementPath.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zHeistInteractionObject.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zHeistNPCInterface.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zHumanVehiclePlayer.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zHumanVehiclePlayerActions.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zNPCBase.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zNPCManager.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNPCTypesIN2.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCommonPlayerActionPushPull.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zPushableObject.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCompSteeringBasic.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNPCChef.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNPCThiefOld.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zNPCThief.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zMiniGameBase.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zMiniGameManager.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNPCSwarmMember.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zMiniGamePlayer.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zMiniME.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zRatBallPlayer.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zRatBallPlayerActions.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zMiniGameDefendTheBase.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zMission.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zNPCAlarmer.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCompStateMachine.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zUICustomImageSlideshow.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zUICustomCheatsManager.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zCompLogicSwarmMember.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNPCWaiter.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCompLogicWaiter.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNPCStirringPlayer.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNPCStirringRemy.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zCompLogicStirringPlayer.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCompLogicStirringRemy.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCompSteeringFlocking.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zPlayerLocationPlatform.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zMiniGameRatOnBall.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zMiniGameRatOnBallPlayer.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zMiniGameDefendTheBasePlayer.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNPCLeftArm.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNPCRightArm.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCompLogicLeftArm.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCompLogicRightArm.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNPCSpringBoard.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCompLogicSpringBoard.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zModuleMgr_Registry.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zModuleExample.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zModuleMgr.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zCompParameters.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zMiniGameGrapeStomper.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zMiniGameGrapeStomperPlayer.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zMiniGamePufferoids.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zMiniGamePufferoidsPlayer.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zMiniGameCongaLine.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zCompLogicPourNSwirl.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNPCPourNSwirl.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zMiniGamePickups.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zICompNPCEntity.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zHUDCompass.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNPCContextSensitive.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCompLogicContextSensitive.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCompLogicAnimViewer.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNPCAnimViewer.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNPCStates.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zMiniGameAvoidTheStuffPlayer.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zButtonMasherWidget.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNPCWalls.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zRaftTrack.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zCompSteeringFlockingBrain.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCompLogicSimpleton.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNPCSimpleton.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zNPCGate.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zPourStream.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zCompSound.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCompLogicSwarmBug.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCompLogicSwarmOwl.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zNPCSwarmBug.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zNPCSwarmOwl.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zComponentTypes.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zDebugTweakBranch.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zCompCombat.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCompSteeringGeneric.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zNPCInstaKill.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCompPerception.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zCompPerceptionParams.cpp", sym="on"),
            GameObject(NonMatching, "rt/Engine/Game/zCompLogicInstaKill.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zSweptCircle.cpp", sym="on"),
            GameObject(Matching, "rt/Engine/Game/zCompLogicAlarmer.cpp", sym="on"),
        ],
    },
    {
        "lib": "binkngc",
        "mw_version": "GC/1.3.2",
        "cflags": cflags_runtime,
        "progress_category": "sdk",
        "objects": [
            Object(NonMatching, "bink/src/sdk/decode/ngc/binkngc.c"),
            Object(NonMatching, "bink/src/sdk/decode/ngc/ngcsnd.c"),
            Object(NonMatching, "bink/src/sdk/decode/binkread.c"),
            Object(NonMatching, "bink/src/sdk/decode/ngc/ngcfile.c"),
            Object(NonMatching, "bink/shared/memory/ngc/radmem.c"),
            Object(NonMatching, "bink/src/sdk/decode/yuv.c"),
            Object(NonMatching, "bink/src/sdk/decode/binkacd.c"),
            Object(NonMatching, "bink/shared/time/radcb.c"),
            Object(NonMatching, "bink/src/sdk/decode/expand.c"),
            Object(NonMatching, "bink/src/sdk/popmal.c"),
            Object(NonMatching, "bink/src/sdk/decode/ngc/ngcrgb.c"),
            Object(NonMatching, "bink/src/sdk/decode/ngc/ngcyuy2.c"),
            Object(NonMatching, "bink/src/sdk/fft.c"),
            Object(NonMatching, "bink/src/sdk/dct.c"),
            Object(NonMatching, "bink/src/sdk/bitplane.c"),
        ],
    },
    {
        "lib": "fmod",
        "mw_version": "GC/1.3.2",
        "cflags": cflags_runtime,
        "progress_category": "sdk",
        "objects": [
            Object(NonMatching, "fmod/fmusic.c"),
            Object(NonMatching, "fmod/format_fsb.c"),
            Object(NonMatching, "fmod/format_gcadpcm.c"),
            Object(NonMatching, "fmod/fsound.c"),
            Object(NonMatching, "fmod/fsound_3d.c"),
            Object(NonMatching, "fmod/fsound_async.c"),
            Object(NonMatching, "fmod/fsound_channel.c"),
            Object(NonMatching, "fmod/fsound_device.c"),
            Object(NonMatching, "fmod/fsound_dsp.c"),
            Object(NonMatching, "fmod/fsound_gc.c"),
            Object(NonMatching, "fmod/fsound_sample.c"),
            Object(NonMatching, "fmod/fsound_stream.c"),
            Object(NonMatching, "fmod/mixer.c"),
            Object(NonMatching, "fmod/fsound_stream_decode.c"),
            Object(NonMatching, "fmod/mixer_clipcopy.c"),
            Object(NonMatching, "fmod/music_formatfsb.c"),
            Object(NonMatching, "fmod/output_gc.c"),
            Object(NonMatching, "fmod/output_nosound.c"),
            Object(NonMatching, "fmod/sound_software.c"),
            Object(NonMatching, "fmod/sound_software_3d.c"),
            Object(NonMatching, "fmod/system_file.c"),
            Object(NonMatching, "fmod/system_memory.c"),
            Object(NonMatching, "fmod/system_misc_gc.c"),
            Object(NonMatching, "fmod/system_thread.c"),
            Object(NonMatching, "fmod/system_time.c"),
            Object(NonMatching, "fmod/system_string.c"),
        ],
    },
    {
        "lib": "Runtime.PPCEABI.H",
        "mw_version": config.linker_version,
        "cflags": cflags_runtime,
        "progress_category": "sdk",
        "objects": [
            Object(NonMatching, "PowerPC_EABI_Support/Runtime/Src/__va_arg.c"),
            Object(NonMatching, "PowerPC_EABI_Support/Runtime/Src/global_destructor_chain.c"),
            Object(NonMatching, "PowerPC_EABI_Support/Runtime/Src/__mem.c"),
            Object(NonMatching, "PowerPC_EABI_Support/Runtime/Src/NewMore.cp"),
            Object(NonMatching, "PowerPC_EABI_Support/Runtime/Src/CPlusLibPPC.cp"),
            Object(NonMatching, "PowerPC_EABI_Support/Runtime/Src/New.cp"),
            Object(NonMatching, "PowerPC_EABI_Support/Runtime/Src/NMWException.cp"),
            Object(NonMatching, "PowerPC_EABI_Support/Runtime/Src/ptmf.c"),
            Object(NonMatching, "PowerPC_EABI_Support/Runtime/Src/runtime.c"),
            Object(NonMatching, "PowerPC_EABI_Support/Runtime/Src/__init_cpp_exceptions.cpp"),
            Object(NonMatching, "PowerPC_EABI_Support/Runtime/Src/Gecko_ExceptionPPC.cp"),
            Object(NonMatching, "PowerPC_EABI_Support/Runtime/Src/GCN_mem_alloc.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/abort_exit.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/alloc.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/errno.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/ansi_files.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Src/ansi_fp.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/arith.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/assert.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/bsearch.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/buffer_io.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/PPC_EABI/SRC/critical_regions.gamecube.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/ctype.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/locale.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/direct_io.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/file_io.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/FILE_POS.C"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/mbstring.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/mem.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/mem_funcs.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/misc_io.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/printf.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/qsort.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/rand.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/scanf.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/signal.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/string.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/float.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/strtold.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/strtoul.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/time.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/wchar_io.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/PPC_EABI/SRC/time.dolphin.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/PPC_EABI/SRC/uart_console_io_gcn.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/e_acos.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/e_asin.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/e_atan2.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/e_exp.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/e_fmod.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/e_log.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/e_pow.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/e_rem_pio2.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/k_cos.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/k_rem_pio2.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/k_sin.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/k_tan.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_atan.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_ceil.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_copysign.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_cos.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_floor.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_frexp.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_ldexp.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_modf.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_sin.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_tan.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/w_acos.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/w_asin.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/w_atan2.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/w_exp.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/w_fmod.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/w_log.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/w_pow.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/e_sqrt.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/PPC_EABI/SRC/math_ppc.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/w_sqrt.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/extras.c"),
        ],
    },
    {
        "lib": "TRK_Minnow_Dolphin",
        "mw_version": config.linker_version,
        "cflags": cflags_runtime,
        "progress_category": "sdk",
        "objects": [
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Portable/mainloop.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Portable/nubevent.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Portable/nubinit.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Portable/msg.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Portable/msgbuf.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Portable/serpoll.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Os/dolphin/usr_put.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Portable/dispatch.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Portable/msghndlr.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Portable/support.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Portable/mutex_TRK.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Portable/notify.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Processor/ppc/Generic/flush_cache.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Portable/mem_TRK.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Processor/ppc/Generic/targimpl.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Processor/ppc/Export/targsupp.s"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Processor/ppc/Generic/mpc_7xx_603e.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Processor/ppc/Generic/__exception.s"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Os/dolphin/dolphin_trk.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Portable/main_TRK.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Os/dolphin/dolphin_trk_glue.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Os/dolphin/targcont.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Os/dolphin/target_options.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Export/mslsupp.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/debugger/embedded/MetroTRK/Os/dolphin/UDP_Stubs.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/gamedev/cust_connection/cc/exi2/GCN/EXI2_DDH_GCN/main.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/gamedev/cust_connection/utils/common/CircleBuffer.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/gamedev/cust_connection/cc/exi2/GCN/EXI2_GDEV_GCN/main.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/gamedev/cust_connection/utils/common/MWTrace.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MetroTRK/gamedev/cust_connection/utils/gc/MWCriticalSection_gc.cpp"),
        ],
    },
    DolphinLib(
        "ai",
        [
            Object(Matching, "dolphin/src/ai/ai.c"),
        ],
    ),
    DolphinLib(
        "amcstubs",
        [
            Object(Matching, "dolphin/src/amcstubs/AmcExi2Stubs.c"),
        ],
    ),
    DolphinLib(
        "ar",
        [
            Object(Matching, "dolphin/src/ar/ar.c"),
            Object(Matching, "dolphin/src/ar/arq.c"),
        ],
    ),
    DolphinLib(
        "ax",
        [
            Object(Matching, "dolphin/src/ax/AX.c"),
            Object(Matching, "dolphin/src/ax/AXAlloc.c"),
            Object(Matching, "dolphin/src/ax/AXAux.c"),
            Object(Matching, "dolphin/src/ax/AXCL.c"),
            Object(Matching, "dolphin/src/ax/AXOut.c"),
            Object(Matching, "dolphin/src/ax/AXSPB.c"),
            Object(Matching, "dolphin/src/ax/AXVPB.c"),
            Object(Matching, "dolphin/src/ax/AXComp.c"),
            Object(Matching, "dolphin/src/ax/DSPCode.c"),
            Object(Matching, "dolphin/src/ax/AXProf.c"),
        ],
    ),
    DolphinLib(
        "axfx",
        [
            Object(Matching, "dolphin/src/axfx/reverb_hi.c"),
            Object(Matching, "dolphin/src/axfx/reverb_std.c"),
            Object(Matching, "dolphin/src/axfx/chorus.c"),
            Object(Matching, "dolphin/src/axfx/delay.c"),
            Object(Matching, "dolphin/src/axfx/axfx.c"),
            Object(Matching, "dolphin/src/axfx/reverb_hi_4ch.c"),
        ],
    ),
    DolphinLib(
        "base",
        [
            Object(Matching, "dolphin/src/base/PPCArch.c"),
        ],
    ),
    DolphinLib(
        "card",
        [
            Object(Matching, "dolphin/src/card/CARDBios.c"),
            Object(Matching, "dolphin/src/card/CARDUnlock.c"),
            Object(Matching, "dolphin/src/card/CARDRdwr.c"),
            Object(Matching, "dolphin/src/card/CARDBlock.c"),
            Object(Matching, "dolphin/src/card/CARDDir.c"),
            Object(Matching, "dolphin/src/card/CARDCheck.c"),
            Object(Matching, "dolphin/src/card/CARDMount.c"),
            Object(Matching, "dolphin/src/card/CARDFormat.c"),
            Object(Matching, "dolphin/src/card/CARDOpen.c", extra_cflags=sdk_char_signed),
            Object(Matching, "dolphin/src/card/CARDCreate.c"),
            Object(Matching, "dolphin/src/card/CARDRead.c"),
            Object(Matching, "dolphin/src/card/CARDWrite.c"),
            Object(Matching, "dolphin/src/card/CARDDelete.c"),
            Object(Matching, "dolphin/src/card/CARDStat.c"),
            Object(Matching, "dolphin/src/card/CARDStatEx.c"),
            Object(Matching, "dolphin/src/card/CARDNet.c"),
        ],
    ),
    DolphinLib(
        "db",
        [
            Object(Matching, "dolphin/src/db/db.c"),
        ],
    ),
    DolphinLib(
        "dsp",
        [
            Object(Matching, "dolphin/src/dsp/dsp.c"),
            Object(Matching, "dolphin/src/dsp/dsp_debug.c"),
            Object(Matching, "dolphin/src/dsp/dsp_task.c"),
        ],
    ),
    DolphinLib(
        "dvd",
        [
            Object(Matching, "dolphin/src/dvd/dvdlow.c", extra_cflags=sdk_char_signed),
            Object(Matching, "dolphin/src/dvd/dvdfs.c", extra_cflags=sdk_char_signed),
            Object(Matching, "dolphin/src/dvd/dvd.c", extra_cflags=sdk_char_signed),
            Object(Matching, "dolphin/src/dvd/dvdqueue.c", extra_cflags=sdk_char_signed),
            Object(Matching, "dolphin/src/dvd/dvderror.c", extra_cflags=sdk_char_signed),
            Object(Matching, "dolphin/src/dvd/dvdidutils.c", extra_cflags=sdk_char_signed),
            Object(Matching, "dolphin/src/dvd/dvdFatal.c", extra_cflags=sdk_char_signed),
            Object(Matching, "dolphin/src/dvd/fstload.c", extra_cflags=sdk_char_signed),
        ],
    ),
    DolphinLib(
        "exi",
        [
            Object(Matching, "dolphin/src/exi/EXIBios.c", cflags=cflags_sdk_exibios),
            Object(Matching, "dolphin/src/exi/EXIUart.c"),
        ],
    ),
    DolphinLib(
        "gx",
        [
            Object(Matching, "dolphin/src/gx/GXInit.c"),
            Object(MatchingFor("RELEASE", "MASTERDEBUG", "MASTER"), "dolphin/src/gx/GXFifo.c"),
            Object(Matching, "dolphin/src/gx/GXAttr.c"),
            Object(Matching, "dolphin/src/gx/GXMisc.c"),
            Object(Matching, "dolphin/src/gx/GXGeometry.c"),
            Object(Matching, "dolphin/src/gx/GXFrameBuf.c"),
            Object(Matching, "dolphin/src/gx/GXLight.c"),
            Object(Matching, "dolphin/src/gx/GXTexture.c"),
            Object(Matching, "dolphin/src/gx/GXBump.c"),
            Object(Matching, "dolphin/src/gx/GXTev.c"),
            Object(Matching, "dolphin/src/gx/GXPixel.c"),
            Object(Matching, "dolphin/src/gx/GXDisplayList.c"),
            Object(Matching, "dolphin/src/gx/GXVert.c"),
            Object(Matching, "dolphin/src/gx/GXTransform.c"),
            Object(Matching, "dolphin/src/gx/GXVerify.c"),
            Object(NonMatching, "dolphin/src/gx/GXVerifXF.c"),
            Object(NonMatching, "dolphin/src/gx/GXVerifRAS.c"),
            Object(Matching, "dolphin/src/gx/GXSave.c"),
            Object(Matching, "dolphin/src/gx/GXPerf.c"),
        ],
    ),
    DolphinLib(
        "hio",
        [
            Object(Matching, "dolphin/src/hio/hio.c"),
        ],
    ),
    DolphinLib(
        "mix",
        [
            Object(Matching, "dolphin/src/mix/mix.c"),
        ],
    ),
    DolphinLib(
        "mtx",
        [
            Object(Matching, "dolphin/src/mtx/mtx.c", extra_cflags=sdk_char_signed),
            Object(Matching, "dolphin/src/mtx/mtx44.c", extra_cflags=sdk_char_signed),
        ],
    ),
    DolphinLib(
        "odenotstub",
        [
            Object(Matching, "dolphin/src/odenotstub/odenotstub.c"),
        ],
    ),
    DolphinLib(
        "os",
        [
            Object(Matching, "dolphin/src/os/OS.c"),
            Object(Matching, "dolphin/src/os/OSAddress.c"),
            Object(Matching, "dolphin/src/os/OSAlarm.c"),
            Object(Matching, "dolphin/src/os/OSAlloc.c"),
            Object(Matching, "dolphin/src/os/OSArena.c"),
            Object(Matching, "dolphin/src/os/OSAudioSystem.c"),
            Object(Matching, "dolphin/src/os/OSCache.c"),
            Object(Matching, "dolphin/src/os/OSContext.c"),
            Object(Matching, "dolphin/src/os/OSError.c"),
            Object(Matching, "dolphin/src/os/OSExec.c"),
            Object(Matching, "dolphin/src/os/OSFont.c"),
            Object(Matching, "dolphin/src/os/OSInterrupt.c"),
            Object(Matching, "dolphin/src/os/OSLink.c"),
            Object(Matching, "dolphin/src/os/OSMemory.c"),
            Object(Matching, "dolphin/src/os/OSMutex.c"),
            Object(Matching, "dolphin/src/os/OSReboot.c"),
            Object(Matching, "dolphin/src/os/OSReset.c"),
            Object(Matching, "dolphin/src/os/OSResetSW.c"),
            Object(Matching, "dolphin/src/os/OSRtc.c"),
            Object(Matching, "dolphin/src/os/OSSemaphore.c"),
            Object(Matching, "dolphin/src/os/OSSync.c"),
            Object(Matching, "dolphin/src/os/OSThread.c"),
            Object(Matching, "dolphin/src/os/OSTime.c"),
            Object(Matching, "dolphin/src/os/__start.c"),
            Object(Matching, "dolphin/src/os/__ppc_eabi_init.cpp"),
        ],
    ),
    DolphinLib(
        "pad",
        [
            Object(Matching, "dolphin/src/pad/Padclamp.c"),
            Object(Matching, "dolphin/src/pad/Pad.c"),
        ],
    ),
    DolphinLib(
        "si",
        [
            Object(Matching, "dolphin/src/si/SIBios.c"),
            Object(Matching, "dolphin/src/si/SISamplingRate.c"),
        ],
    ),
    DolphinLib(
        "vi",
        [
            Object(Matching, "dolphin/src/vi/vi.c"),
            Object(Matching, "dolphin/src/vi/i2c.c"),
            Object(Matching, "dolphin/src/vi/initphilips.c"),
            Object(Matching, "dolphin/src/vi/gpioexi.c"),
        ],
    ),
    {
        "lib": "OdemuExi2",
        "mw_version": "GC/1.3.2",
        "cflags": cflags_runtime,
        "progress_category": "sdk",
        "objects": [
            Object(NonMatching, "OdemuExi2/DebuggerDriver.c"),
        ],
    },
    RenderWareLib(
        "rpcollis",
        cflags_rpcollis,
        [
            Object(NonMatching, "rwsdk/plugin/collis/ctdata.c"),
            Object(NonMatching, "rwsdk/plugin/collis/ctquery.c"),
            Object(NonMatching, "rwsdk/plugin/collis/colldata.c"),
            Object(NonMatching, "rwsdk/plugin/collis/collquery.c"),
            Object(NonMatching, "rwsdk/include/gcn/rpdbgerr_1.c"),
        ],
    ),
    RenderWareLib(
        "rphanim",
        cflags_rphanim,
        [
            Object(NonMatching, "rwsdk/plugin/hanim/stdkey.c"),
            Object(NonMatching, "rwsdk/plugin/hanim/rphanim.c"),
        ],
    ),
    RenderWareLib(
        "rpmatfx",
        cflags_rpmatfx,
        [
            Object(NonMatching, "rwsdk/plugin/matfx/rpmatfx.c"),
            Object(NonMatching, "rwsdk/plugin/matfx/gcn/effectPipesGcn.c"),
            Object(NonMatching, "rwsdk/plugin/matfx/gcn/multiTexGcnData.c"),
            Object(NonMatching, "rwsdk/plugin/matfx/gcn/multiTexGcnPipe.c"),
            Object(NonMatching, "rwsdk/plugin/matfx/gcn/multiTexGcn.c"),
            Object(NonMatching, "rwsdk/plugin/matfx/gcn/nbtGen.c"),
            Object(NonMatching, "rwsdk/plugin/matfx/multiTex.c"),
            Object(NonMatching, "rwsdk/plugin/matfx/multiTexEffect.c"),
            Object(NonMatching, "rwsdk/include/gcn/rpdbgerr_2.c"),
        ],
    ),
    RenderWareLib(
        "rpptank",
        cflags_rpptank,
        [
            Object(NonMatching, "rwsdk/plugin/ptank/rpptank.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcn.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcncallbacks.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcnrender.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcntransforms.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcn_nc_ppm.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcn_cc_ppm.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcn_nc_cs_nr.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcn_cc_cs_nr.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcn_nc_pps_nr.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcn_cc_pps_nr.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcn_nc_cs_ppr.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcn_cc_cs_ppr.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcn_nc_pps_ppr.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcn_cc_pps_ppr.c"),
        ],
    ),
    RenderWareLib(
        "rpskinmatfx",
        cflags_rpskinmatfx,
        [
            Object(NonMatching, "rwsdk/plugin/skin2/bsplit.c"),
            Object(NonMatching, "rwsdk/plugin/skin2/rpskin.c"),
            Object(NonMatching, "rwsdk/plugin/skin2/gcn/skingcn.c"),
            Object(NonMatching, "rwsdk/plugin/skin2/gcn/skinstream.c"),
            Object(NonMatching, "rwsdk/plugin/skin2/gcn/instance/instanceskin.c"),
            Object(NonMatching, "rwsdk/plugin/skin2/gcn/skinmatrixblend.c"),
            Object(NonMatching, "rwsdk/plugin/skin2/gcn/skingcnasm.c"),
            Object(NonMatching, "rwsdk/plugin/skin2/gcn/skinmatfxgcn.c"),
            Object(NonMatching, "rwsdk/plugin/skin2/gcn/skingcnm.c"),
            Object(NonMatching, "rwsdk/include/gcn/rpdbgerr_3.c"),
        ],
    ),
    RenderWareLib(
        "rpusrdat",
        cflags_rpusrdat,
        [
            # Release seems to have an older version of rpusrdat for some reason.
            Object(NonMatching, "rwsdk/plugin/userdata/debug/rpusrdat.c") if version_num == 0 else
            Object(NonMatching, "rwsdk/plugin/userdata/release/rpusrdat.c"),
        ],
    ),
    RenderWareLib(
        "rpworld",
        cflags_rpworld,
        [
            Object(NonMatching, "rwsdk/world/babinwor.c"),
            Object(NonMatching, "rwsdk/world/baclump.c"),
            Object(NonMatching, "rwsdk/world/bageomet.c"),
            Object(NonMatching, "rwsdk/world/balight.c"),
            Object(NonMatching, "rwsdk/world/bamateri.c"),
            Object(NonMatching, "rwsdk/world/bamatlst.c"),
            Object(NonMatching, "rwsdk/world/bamesh.c"),
            Object(NonMatching, "rwsdk/world/bameshop.c"),
            Object(NonMatching, "rwsdk/world/basector.c"),
            Object(NonMatching, "rwsdk/world/baworld.c"),
            Object(NonMatching, "rwsdk/world/baworobj.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/bapipew.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/gcpipe.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/vtxfmt.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/wrldpipe.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/nodeGameCubeAtomicAllInOne.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/nodeGameCubeWorldSectorAllInOne.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/gclights.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/gcmorph.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/native.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/setup.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/instance/geomcond.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/instance/geominst.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/instance/ibuffer.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/instance/instancegeom.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/instance/instanceworld.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/instance/itools.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/instance/vbuffer.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/instance/vtools.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/instance/vtxdesc.c"),
            Object(NonMatching, "rwsdk/include/gcn/rpdbgerr_4.c"),
        ],
    ),
    RenderWareLib(
        "rtanim",
        cflags_rtanim,
        [
            Object(NonMatching, "rwsdk/tool/anim/rtanim.c"),
            Object(NonMatching, "rwsdk/include/gcn/rpdbgerr_5.c"),
        ],
    ),
    RenderWareLib(
        "rtcharse",
        cflags_rtcharse,
        [
            Object(NonMatching, "rwsdk/tool/charse/chrprint.c"),
            Object(NonMatching, "rwsdk/tool/charse/rtcharse.c"),
            Object(NonMatching, "rwsdk/include/gcn/rpdbgerr_6.c"),
        ],
    ),
    RenderWareLib(
        "rwcore",
        cflags_rwcore,
        [
            Object(NonMatching, "rwsdk/src/plcore/babinary.c"),
            Object(NonMatching, "rwsdk/src/plcore/bacolor.c"),
            Object(NonMatching, "rwsdk/src/plcore/badebug.c"),
            Object(NonMatching, "rwsdk/src/plcore/baerr.c"),
            Object(NonMatching, "rwsdk/src/plcore/baimmedi.c"),
            Object(NonMatching, "rwsdk/src/plcore/bamatrix.c"),
            Object(NonMatching, "rwsdk/src/plcore/bamemory.c"),
            Object(NonMatching, "rwsdk/src/plcore/baresour.c"),
            Object(NonMatching, "rwsdk/src/plcore/bastream.c"),
            Object(NonMatching, "rwsdk/src/plcore/batkbin.c"),
            Object(NonMatching, "rwsdk/src/plcore/batkreg.c"),
            Object(NonMatching, "rwsdk/src/plcore/bavector.c"),
            Object(NonMatching, "rwsdk/src/plcore/resmem.c"),
            Object(NonMatching, "rwsdk/src/plcore/rwdbgerr.c"),
            Object(NonMatching, "rwsdk/src/plcore/rwgrp.c"),
            Object(NonMatching, "rwsdk/src/plcore/rwstring.c"),
            Object(NonMatching, "rwsdk/os/gcn/osintf.c"),
            Object(NonMatching, "rwsdk/src/babbox.c"),
            Object(NonMatching, "rwsdk/src/babincam.c"),
            Object(NonMatching, "rwsdk/src/babinfrm.c"),
            Object(NonMatching, "rwsdk/src/babintex.c"),
            Object(NonMatching, "rwsdk/src/bacamera.c"),
            Object(NonMatching, "rwsdk/src/badevice.c"),
            Object(NonMatching, "rwsdk/src/baframe.c"),
            Object(NonMatching, "rwsdk/src/baimage.c"),
            Object(NonMatching, "rwsdk/src/baimras.c"),
            Object(NonMatching, "rwsdk/src/baraster.c"),
            Object(NonMatching, "rwsdk/src/baresamp.c"),
            Object(NonMatching, "rwsdk/src/basync.c"),
            Object(NonMatching, "rwsdk/src/batextur.c"),
            Object(NonMatching, "rwsdk/src/batypehf.c"),
            Object(NonMatching, "rwsdk/driver/common/palquant.c"),
            Object(NonMatching, "rwsdk/driver/gcn/dl2drend.c"),
            Object(NonMatching, "rwsdk/driver/gcn/dlconvrt.c"),
            Object(NonMatching, "rwsdk/driver/gcn/dldevice.c"),
            Object(NonMatching, "rwsdk/driver/gcn/dlraster.c"),
            Object(NonMatching, "rwsdk/driver/gcn/dlrendst.c"),
            Object(NonMatching, "rwsdk/driver/gcn/dlsprite.c"),
            Object(NonMatching, "rwsdk/driver/gcn/dltexdic.c"),
            Object(NonMatching, "rwsdk/driver/gcn/dltextur.c"),
            Object(NonMatching, "rwsdk/driver/gcn/dltoken.c"),
            Object(NonMatching, "rwsdk/src/pipe/p2/baim3d.c"),
            Object(NonMatching, "rwsdk/src/pipe/p2/bapipe.c"),
            Object(NonMatching, "rwsdk/src/pipe/p2/p2altmdl.c"),
            Object(NonMatching, "rwsdk/src/pipe/p2/p2core.c"),
            Object(NonMatching, "rwsdk/src/pipe/p2/p2define.c"),
            Object(NonMatching, "rwsdk/src/pipe/p2/p2dep.c"),
            Object(NonMatching, "rwsdk/src/pipe/p2/p2heap.c"),
            Object(NonMatching, "rwsdk/src/pipe/p2/p2renderstate.c"),
            Object(NonMatching, "rwsdk/src/pipe/p2/p2resort.c"),
            Object(NonMatching, "rwsdk/src/pipe/p2/gcn/im3dpipe.c"),
            Object(NonMatching, "rwsdk/src/pipe/p2/gcn/nodeDolphinSubmitNoLight.c"),
        ],
    ),
    RenderWareLib(
        "rtfsyst",
        cflags_rtfsyst,
        [
            # Release seems to have a different (older?) version of rtfsyst for some reason.
            Object(NonMatching, "rwsdk/tool/fsyst/debug/rtfsmgr.c") if version_num == 0 else
            Object(NonMatching, "rwsdk/tool/fsyst/release/rtfsmgr.c"),
        ],
    ),
    RenderWareLib(
        "rtintsec",
        cflags_rtintsec,
        [
            Object(NonMatching, "rwsdk/tool/intsec/rtintsec.c"),
        ],
    ),
    RenderWareLib(
        "rtquat",
        cflags_rtquat,
        [
            Object(NonMatching, "rwsdk/tool/quat/rtquat.c"),
        ],
    ),
    RenderWareLib(
        "rtskinsp",
        cflags_rtskinsp,
        [
            Object(NonMatching, "rwsdk/tool/sknsplit/skinsplit.c"),
            Object(NonMatching, "rwsdk/tool/sknsplit/rtskinsp.c"),
        ],
    ),
    RenderWareLib(
        "rtslerp",
        cflags_rtslerp,
        [
            Object(NonMatching, "rwsdk/tool/slerp/rtslerp.c"),
        ],
    ),
    RenderWareLib(
        "rttilerd",
        cflags_rttilerd,
        [
            Object(NonMatching, "rwsdk/tool/tilerd/rttilerd.c"),
        ],
    ),
    DolphinLib(
        "kpad",
        [
            Object(NonMatching, "revolution/kpad/src/KPAD.c"),
        ],
    ),
    DolphinLib(
        "wpad",
        [
            Object(NonMatching, "revolution/wpad/src/wpadv3.c"),
        ],
    ),
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
]
config.progress_each_module = args.verbose
# Optional extra arguments to `objdiff-cli report generate`
config.progress_report_args = [
    # Marks relocations as mismatching if the target value is different
    # Default is "functionRelocDiffs=none", which is most lenient
    # "--config functionRelocDiffs=data_value",
]

if args.mode == "configure":
    # Write build.ninja and objdiff.json
    generate_build(config)
elif args.mode == "progress":
    # Print progress information
    calculate_progress(config)
else:
    sys.exit("Unknown mode: " + args.mode)
