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
    "-i src",
    f"-i build/{config.version}/include",
    "-i src/PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Include",
    #"-i src/PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Include",
    #"-i src/PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Include",
    #"-i src/PowerPC_EABI_Support/MSL/MSL_C/PPC_EABI/Include",
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
    "-i src/bink/bink",
    "-DDEBUG=0",
    "-DRELEASE=0",
    "-DMASTERDEBUG=0",
    "-DMASTER=0",
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
cflags_rat_dbg = [
    *cflags_rat,
    "-sym on",
]

config.linker_version = "GC/2.6"

# Game code prefix: include/prefix.pch++, precompiled per build with the game flags into
# build/<version>/include/prefix.mch (found through -i build/<version>/include) and passed to
# every game unit with -prefix (cflags_rat_base).
config.precompiled_headers = [
    {
        "source": "prefix.pch++",
        "mw_version": config.linker_version,
        "cflags": ["-lang=c++", *(f for f in cflags_rat if f != "-prefix prefix.mch")],
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


config.warn_missing_config = True
config.warn_missing_source = False
config.libs = [
    {
        "lib": "RT",
        "mw_version": config.linker_version,
        "cflags": cflags_rat,
        "progress_category": "game",
        "objects": [
            Object(NonMatching, "rt/Engine/Core/x/xAnim.cpp"),
            Object(Matching, "rt/Engine/Core/x/xBase.cpp"),
            Object(Matching, "rt/Engine/Core/x/xbinio.cpp"),
            Object(Matching, "rt/Engine/Core/x/xBound.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xCam.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xCamFollow.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xCamSupport.cpp"),
            Object(Matching, "rt/Engine/Core/x/xClimate.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xClumpColl.cpp"),
            Object(Matching, "rt/Engine/Core/x/xCM.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xCollide.cpp"),
            Object(Matching, "rt/Engine/Core/x/xCollideFast.cpp"),
            Object(Matching, "rt/Engine/Core/x/xColor.cpp"),
            Object(Matching, "rt/Engine/Core/x/xCounter.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xCutscene.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xDebug.cpp"),
            Object(Matching, "rt/Engine/Core/x/xDebugGraph.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xDebugTweak.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xDecal.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xDraw.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xEnt.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xEntBoulder.cpp"),
            Object(Matching, "rt/Engine/Core/x/xEntDrive.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xEntMotion.cpp"),
            Object(Matching, "rt/Engine/Core/x/xEnv.cpp"),
            Object(Matching, "rt/Engine/Core/x/xEvent.cpp"),
            Object(Matching, "rt/Engine/Core/x/xFactory.cpp"),
            Object(Matching, "rt/Engine/Core/x/xFFX.cpp"),
            Object(Matching, "rt/Engine/Core/x/xFilter.cpp"),
            Object(Matching, "rt/Engine/Core/x/xFMV.cpp"),
            Object(Matching, "rt/Engine/Core/x/xFog.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xFont.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xFX.cpp"),
            Object(Matching, "rt/Engine/Core/x/xGrid.cpp"),
            Object(Matching, "rt/Engine/Core/x/xGroup.cpp"),
            Object(Matching, "rt/Engine/Core/x/xHierarchyBound.cpp"),
            Object(Matching, "rt/Engine/Core/x/xhipio.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xHud.cpp"),
            Object(Matching, "rt/Engine/Core/x/xHudFontMeter.cpp"),
            Object(Matching, "rt/Engine/Core/x/xHudMeter.cpp"),
            Object(Matching, "rt/Engine/Core/x/xHudModel.cpp"),
            Object(Matching, "rt/Engine/Core/x/xHudUnitMeter.cpp"),
            Object(Matching, "rt/Engine/Core/x/xIni.cpp"),
            Object(Matching, "rt/Engine/Core/x/xJaw.cpp"),
            Object(Matching, "rt/Engine/Core/x/xJSP.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xLight.cpp"),
            Object(Matching, "rt/Engine/Core/x/xLightKit.cpp"),
            Object(Matching, "rt/Engine/Core/x/xMath.cpp"),
            Object(Matching, "rt/Engine/Core/x/xMath2.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xMath3.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xMemMgr.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xModel.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xModelBucket.cpp"),
            Object(Matching, "rt/Engine/Core/x/xMorph.cpp"),
            Object(Matching, "rt/Engine/Core/x/xMovePoint.cpp"),
            Object(Matching, "rt/Engine/Core/x/xNavigationMeshAsset.cpp"),
            Object(Matching, "rt/Engine/Core/x/xordarray.cpp"),
            Object(Matching, "rt/Engine/Core/x/xOutputMgr.cpp"),
            Object(Matching, "rt/Engine/Core/x/xPad.cpp"),
            Object(Matching, "rt/Engine/Core/x/xPar.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xParCmd.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xParEmitter.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xParEmitterType.cpp"),
            Object(Matching, "rt/Engine/Core/x/xParGroup.cpp"),
            Object(Matching, "rt/Engine/Core/x/xParMgr.cpp"),
            Object(Matching, "rt/Engine/Core/x/xParSys.cpp"),
            Object(Matching, "rt/Engine/Core/x/xPartition.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xpkrsvc.cpp"),
            Object(Matching, "rt/Engine/Core/x/xPtankPool.cpp"),
            Object(Matching, "rt/Engine/Core/x/xQuickCull.cpp"),
            Object(Matching, "rt/Engine/Core/x/xRand.cpp"),
            Object(Matching, "rt/Engine/Core/x/xRegionSupport.cpp"),
            Object(Matching, "rt/Engine/Core/x/xRenderState.cpp"),
            Object(Matching, "rt/Engine/Core/x/xRMemData.cpp"),
            Object(Matching, "rt/Engine/Core/x/xRumbleEmitter.cpp"),
            Object(Matching, "rt/Engine/Core/x/xRumbleManager.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xsavegame.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xScene.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xScreenWarp.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xScrFx.cpp"),
            Object(Matching, "rt/Engine/Core/x/xserializer.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xShadow.cpp"),
            Object(Matching, "rt/Engine/Core/x/xShadowSimple.cpp"),
            Object(Matching, "rt/Engine/Core/x/xSkyDome.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xSndMgr.cpp"),
            Object(Matching, "rt/Engine/Core/x/xSoundFX.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xSpacePartitionTree.cpp"),
            Object(Matching, "rt/Engine/Core/x/xSpline.cpp"),
            Object(Matching, "rt/Engine/Core/x/xSplineAsset.cpp"),
            Object(Matching, "rt/Engine/Core/x/xstransvc.cpp"),
            Object(Matching, "rt/Engine/Core/x/xString.cpp"),
            Object(Matching, "rt/Engine/Core/x/xSubTitles.cpp"),
            Object(Matching, "rt/Engine/Core/x/xSurface.cpp"),
            Object(Matching, "rt/Engine/Core/x/xTextAsset.cpp"),
            Object(Matching, "rt/Engine/Core/x/xTextureManager.cpp"),
            Object(Matching, "rt/Engine/Core/x/xTimer.cpp"),
            Object(Matching, "rt/Engine/Core/x/xTimestamp.cpp"),
            Object(Matching, "rt/Engine/Core/x/xTransient.cpp"),
            Object(Matching, "rt/Engine/Core/x/xTRC.cpp"),
            Object(Matching, "rt/Engine/Core/x/xUpdateCull.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xutil.cpp"),
            Object(Matching, "rt/Engine/Core/x/xVec3.cpp"),
            Object(Matching, "rt/Engine/Core/x/xVolume.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/iAnimSKB.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/iAsync.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/iCutscene.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/iDebug.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/iDraw.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/iEnv.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/iFile.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/iFMV.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/iFX.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/iLight.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/iMath.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/iMath3.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/iMemMgr.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/iModel.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/iMorph.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/iPad.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/iParMgr.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/isavegame.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/iScrFX.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/iSnd.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/iSystem.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/iTime.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/iTRC.cpp"),
            Object(Matching, "rt/Engine/Core/gc/iAnim.cpp"),
            Object(NonMatching, "rt/Engine/Game/zActionLine.cpp"),
            Object(NonMatching, "rt/Engine/Game/zAnimCache.cpp"),
            Object(NonMatching, "rt/Engine/Game/zAnimList.cpp"),
            Object(NonMatching, "rt/Engine/Game/zAssetTypes.cpp"),
            Object(NonMatching, "rt/Engine/Game/zBase.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCheat.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCollGeom.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCommonPlayer.cpp"),
            Object(NonMatching, "rt/Engine/Game/zConditional.cpp"),
            Object(NonMatching, "rt/Engine/Game/zConfiguration.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCutsceneMgr.cpp"),
            Object(NonMatching, "rt/Engine/Game/zDestructible.cpp"),
            Object(NonMatching, "rt/Engine/Game/zDispatcher.cpp"),
            Object(Matching, "rt/Engine/Game/zEditorLink.cpp"),
            Object(NonMatching, "rt/Engine/Game/zEGenerator.cpp"),
            Object(NonMatching, "rt/Engine/Game/zEnt.cpp"),
            Object(NonMatching, "rt/Engine/Game/zEntButton.cpp"),
            Object(NonMatching, "rt/Engine/Game/zEntSimpleObj.cpp"),
            Object(NonMatching, "rt/Engine/Game/zEntTrigger.cpp"),
            Object(NonMatching, "rt/Engine/Game/zEnv.cpp"),
            Object(NonMatching, "rt/Engine/Game/zEvent.cpp"),
            Object(NonMatching, "rt/Engine/Game/zExplosion.cpp"),
            Object(NonMatching, "rt/Engine/Game/zFire.cpp"),
            Object(NonMatching, "rt/Engine/Game/zFlameEmitter.cpp"),
            Object(NonMatching, "rt/Engine/Game/zFMV.cpp"),
            Object(Matching, "rt/Engine/Game/zFX.cpp"),
            Object(NonMatching, "rt/Engine/Game/zFXRibbonPool.cpp"),
            Object(NonMatching, "rt/Engine/Game/zGame.cpp"),
            Object(NonMatching, "rt/Engine/Game/zGameExtras.cpp"),
            Object(NonMatching, "rt/Engine/Game/zGameState.cpp"),
            Object(NonMatching, "rt/Engine/Game/zGrapple.cpp"),
            Object(NonMatching, "rt/Engine/Game/zGrid.cpp"),
            Object(NonMatching, "rt/Engine/Game/zHangable.cpp"),
            Object(NonMatching, "rt/Engine/Game/zHitParameters.cpp"),
            Object(NonMatching, "rt/Engine/Game/zLaserBeam.cpp"),
            Object(NonMatching, "rt/Engine/Game/zLightning.cpp"),
            Object(NonMatching, "rt/Engine/Game/zLightWeight.cpp"),
            Object(NonMatching, "rt/Engine/Game/zLOD.cpp"),
            Object(NonMatching, "rt/Engine/Game/zMain.cpp"),
            Object(NonMatching, "rt/Engine/Game/zMenu.cpp"),
            Object(NonMatching, "rt/Engine/Game/zMovePoint.cpp"),
            Object(NonMatching, "rt/Engine/Game/zMusic.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCMgr.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCNavigation.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCNavigationMesh.cpp"),
            Object(NonMatching, "rt/Engine/Game/zParCmd.cpp"),
            Object(Matching, "rt/Engine/Game/zParEmitter.cpp"),
            Object(NonMatching, "rt/Engine/Game/zPendulum.cpp"),
            Object(NonMatching, "rt/Engine/Game/zPlatform.cpp"),
            Object(NonMatching, "rt/Engine/Game/zPlayer.cpp"),
            Object(NonMatching, "rt/Engine/Game/zPlayerBase.cpp"),
            Object(NonMatching, "rt/Engine/Game/zPortal.cpp"),
            Object(NonMatching, "rt/Engine/Game/zReference.cpp"),
            Object(NonMatching, "rt/Engine/Game/zRenderState.cpp"),
            Object(NonMatching, "rt/Engine/Game/zSaveLoad.cpp"),
            Object(NonMatching, "rt/Engine/Game/zScene.cpp"),
            Object(NonMatching, "rt/Engine/Game/zScript.cpp"),
            Object(NonMatching, "rt/Engine/Game/zShrapnel.cpp"),
            Object(NonMatching, "rt/Engine/Game/zSurface.cpp"),
            Object(Matching, "rt/Engine/Game/zSymbol.cpp"),
            Object(NonMatching, "rt/Engine/Game/zTalkBox.cpp"),
            Object(NonMatching, "rt/Engine/Game/zTaskBox.cpp"),
            Object(NonMatching, "rt/Engine/Game/zTextBox.cpp"),
            Object(Matching, "rt/Engine/Game/zTimer.cpp"),
            Object(Matching, "rt/Engine/Game/zTrainCar.cpp"),
            Object(NonMatching, "rt/Engine/Game/zTransient.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUI.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUIBox.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUIImage.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUIMgr.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUIModel.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUIText.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUIUserString.cpp"),
            Object(NonMatching, "rt/Engine/Game/zVar.cpp"),
            Object(NonMatching, "rt/Engine/Game/zVolume.cpp"),
            Object(NonMatching, "rt/Engine/Game/zWaterhose.cpp"),
            Object(NonMatching, "rt/Engine/Game/zZipLine.cpp"),
        ],
    },
    {  # Compiled with -sym on and not alphabetically sorted
        "lib": "RT_DBG",
        "mw_version": config.linker_version,
        "cflags": cflags_rat_dbg,
        "progress_category": "game",
        "objects": [
            Object(NonMatching, "rt/Engine/Core/gc/screenshot.c"),
            Object(Matching, "rt/Engine/Core/x/xCollideSpherePolygon.cpp"),
            Object(NonMatching, "rt/Engine/Game/zAsync.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xParticleBatch.cpp"),
            Object(NonMatching, "rt/Engine/Game/zSmoke.cpp"),
            Object(NonMatching, "rt/Engine/Game/zSplash.cpp"),
            Object(NonMatching, "rt/Engine/Game/zIncrediSlam.cpp"),
            Object(Matching, "rt/Engine/Core/x/xScrPulseGlare.cpp"),
            Object(NonMatching, "rt/Engine/Game/zBehavior.cpp"),
            Object(NonMatching, "rt/Engine/Game/zWater.cpp"),
            Object(NonMatching, "rt/Engine/Game/zTimeHandler.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xCollideSweptSphere.cpp"),
            Object(Matching, "rt/Engine/Core/x/xScreenFade.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xCollideSphereBox.cpp"),
            Object(NonMatching, "rt/Engine/Game/zReticle.cpp"),
            Object(NonMatching, "rt/Engine/Game/zGrass.cpp"),
            Object(Matching, "rt/Engine/Core/x/xFXCameraTexture.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xFXHighDynamicRange.cpp"),
            Object(Matching, "rt/Engine/Core/x/xCollideSpherePolygonUnprocessed.cpp"),
            Object(NonMatching, "rt/Engine/Game/zInteraction.cpp"),
            Object(NonMatching, "rt/Engine/Game/zBomb.cpp"),
            Object(NonMatching, "rt/Engine/Game/zProgressScript.cpp"),
            Object(Matching, "rt/Engine/Core/x/xHudText.cpp"),
            Object(NonMatching, "rt/Engine/Game/zFlameThrower.cpp"),
            Object(NonMatching, "rt/Engine/Game/zRocket.cpp"),
            Object(NonMatching, "rt/Engine/Core/x/xHudImage.cpp"),
            Object(NonMatching, "rt/Engine/Game/zReactiveAnimation.cpp"),
            Object(NonMatching, "rt/Engine/Game/zDust.cpp"),
            Object(Matching, "rt/Engine/Core/x/xProjectedTexture.cpp"),
            Object(NonMatching, "rt/Engine/Game/zSpotlight.cpp"),
            Object(NonMatching, "rt/Engine/Game/zPickup.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCGroup.cpp"),
            Object(NonMatching, "rt/Engine/Game/zThrowableSystem.cpp"),
            Object(Matching, "rt/Engine/Game/zUIController.cpp"),
            Object(Matching, "rt/Engine/Core/x/xLightVolume.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUICustom.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUIIncredibles.cpp"),
            Object(Matching, "rt/Engine/Core/x/xMemoryManager.cpp"),
            Object(Matching, "rt/Engine/Core/x/xMemoryManagerFixed.cpp"),
            Object(Matching, "rt/Engine/Core/x/xMemoryManagerGeneral.cpp"),
            Object(NonMatching, "rt/Engine/Game/zParticleGenerator.cpp"),
            Object(NonMatching, "rt/Engine/Game/zParticleLocator.cpp"),
            Object(NonMatching, "rt/Engine/Game/zParticleSystems.cpp"),
            Object(NonMatching, "rt/Engine/Game/zParticleSystemWaterfall.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUberLaser.cpp"),
            Object(NonMatching, "rt/Engine/Game/zLoadingScreen.cpp"),
            Object(Matching, "rt/Engine/Core/x/xModelBlur.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/iWater.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/water/fxembm.c"),
            Object(NonMatching, "rt/Engine/Core/gc/water/fxmain.c"),
            Object(NonMatching, "rt/Engine/Core/gc/water/texcache.c"),
            Object(NonMatching, "rt/Engine/Core/gc/water/texconv.c"),
            Object(NonMatching, "rt/Engine/Core/gc/water/textures.c"),
            Object(Matching, "rt/Engine/Game/zEnergyStream.cpp"),
            Object(Matching, "rt/Engine/Game/zWireFrameAsset.cpp"),
            Object(NonMatching, "rt/Engine/Game/zJSPExtraData.cpp"),
            Object(NonMatching, "rt/Engine/Game/zRubbleGenerator.cpp"),
            Object(NonMatching, "rt/Engine/Game/zParticleFXCommon.cpp"),
            Object(Matching, "rt/Engine/Core/x/containers.cpp"),
            Object(Matching, "rt/Engine/Core/x/xFunctionGenerator.cpp"),
            Object(Matching, "rt/Engine/Core/x/xModelWarp.cpp"),
            Object(Matching, "rt/Engine/Game/zCombo.cpp"),
            Object(NonMatching, "rt/Engine/Game/zFrozoneIce.cpp"),
            Object(Matching, "rt/Engine/Core/x/xRenderBuffer.cpp"),
            Object(NonMatching, "rt/Engine/Game/zFXFireTrail.cpp"),
            Object(NonMatching, "rt/Engine/Game/zFXEnergySystem.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUICustomSysMessage.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/iARAMTmp.cpp"),
            Object(NonMatching, "rt/Engine/Game/zSparkEmitter.cpp"),
            Object(NonMatching, "rt/Engine/Game/zToss.cpp"),
            Object(Matching, "rt/Engine/Core/x/xIM.cpp"),
            Object(NonMatching, "rt/Engine/Game/zParticleCustom.cpp"),
            Object(NonMatching, "rt/Engine/Game/zShrapnelEmitter.cpp"),
            Object(NonMatching, "rt/Engine/Game/zSmashRing.cpp"),
            Object(NonMatching, "rt/Engine/Game/zTransientModel.cpp"),
            Object(Matching, "rt/Engine/Core/x/iCamera.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCAsset.cpp"),
            Object(NonMatching, "rt/Engine/Game/zFXLightDust.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCBehavior.cpp"),
            Object(NonMatching, "rt/Engine/Game/zPad.cpp"),
            Object(Matching, "rt/Engine/Game/zNPCDirectMode.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/i2dQuadBuffer.cpp"),
            Object(NonMatching, "rt/Engine/Game/zMeshNavigation.cpp"),
            Object(NonMatching, "rt/Engine/Game/zMeshCollision.cpp"),
            Object(NonMatching, "rt/Engine/Game/zMeshPosition.cpp"),
            Object(Matching, "rt/Engine/Core/x/xLightEffect.cpp"),
            Object(Matching, "rt/Engine/Game/zShowerSystem.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCPerception.cpp"),
            Object(NonMatching, "rt/Engine/Game/zPlayerInput.cpp"),
            Object(NonMatching, "rt/Engine/Game/zPlayerConstrainer.cpp"),
            Object(Matching, "rt/Engine/Game/zNPCSquad.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCommonPlayerActions.cpp"),
            Object(NonMatching, "rt/Engine/Game/zPlayerAction.cpp"),
            Object(NonMatching, "rt/Engine/Game/zAnimCustom.cpp"),
            Object(Matching, "rt/Engine/Core/x/xCamTransition.cpp"),
            Object(NonMatching, "rt/Engine/Game/zFrostEffect.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCCustomAV.cpp"),
            Object(Matching, "rt/Engine/Game/zFrozoneBlast.cpp"),
            Object(NonMatching, "rt/Engine/Game/zParticleSystemSnow.cpp"),
            Object(NonMatching, "rt/Engine/Game/zFXGlint.cpp"),
            Object(NonMatching, "rt/Engine/Game/zParticleSystemRain.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCCoverPoint.cpp"),
            Object(NonMatching, "rt/Engine/Game/zVent.cpp"),
            Object(NonMatching, "rt/Engine/Game/zVentType.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCDodgeList.cpp"),
            Object(Matching, "rt/Engine/Game/zPlayerContainer.cpp"),
            Object(NonMatching, "rt/Engine/Game/zFXGoo.cpp"),
            Object(NonMatching, "rt/Engine/Game/zGoo.cpp"),
            Object(NonMatching, "rt/Engine/Game/zScreenWarp.cpp"),
            Object(NonMatching, "rt/Engine/Game/zFXPunchSnow.cpp"),
            Object(NonMatching, "rt/Engine/Game/zFXPunchStars.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUICustomJoinScreenManager.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCMove.cpp"),
            Object(NonMatching, "rt/Engine/Game/zFXShockwave.cpp"),
            Object(NonMatching, "rt/Engine/Game/zParSysSnowsplosion.cpp"),
            Object(NonMatching, "rt/Engine/Game/zParticleSystemEarthDust.cpp"),
            Object(NonMatching, "rt/Engine/Game/zFXScreenDecal.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUICustomMissionSelect.cpp"),
            Object(NonMatching, "rt/Engine/Core/gc/iDebugLink.cpp"),
            Object(Matching, "rt/Engine/Core/x/xDebugLink.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNavigationMesh.cpp"),
            Object(NonMatching, "rt/Engine/Game/zParticleSystemPop.cpp"),
            Object(Matching, "rt/Engine/Core/x/xOneLiner.cpp"),
            Object(Matching, "rt/Engine/Core/x/xSpringy.cpp"),
            Object(NonMatching, "rt/Engine/Game/zConversation.cpp"),
            Object(NonMatching, "rt/Engine/Game/zParticleSystemBurrowKickup.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUICustomVolume.cpp"),
            Object(Matching, "rt/Engine/Game/zOneLinerSystem.cpp"),
            Object(Equivalent, "rt/Engine/Game/zUIUVs.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCam2Player.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUICustomStatsScreen.cpp"),
            Object(NonMatching, "rt/Engine/Game/zForceFieldEffect.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCheckpoint.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCLevelSpecific.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUICustomCheckpoints.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUICustomControlOverlay.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUICustomLevelManager.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUICustomPauseScreenManager.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUICustomPowerEffect.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUICustomSceneImage.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUICustomScrollers.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUICustomSecretsScreenControl.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUICustomSoundMode.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUICustomStatic.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUICustomUpgrade.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUICustomWireframe.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUICustomArtworkManager.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCFootsteps.cpp"),
            Object(NonMatching, "rt/Engine/Game/zPlayerOneLinerData.cpp"),
            Object(NonMatching, "rt/Engine/Game/zEconomics.cpp"),
            Object(NonMatching, "rt/Engine/Game/zRemyPlayer.cpp"),
            Object(NonMatching, "rt/Engine/Game/zRemyPlayerActions.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCam.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCameraCurve.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCameraFly.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCameraRBand.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCameraTweak.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCamMarker.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCamPlayer.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCamPool.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCamPreset.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCamSpline.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCamSplineCommonMix.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCamTargetSpline.cpp"),
            Object(NonMatching, "rt/Engine/Game/zSlideCamera.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPC.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCCommon.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCCommonBehavior.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCTypes.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCProjectileFrozen.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCProjectileGrub.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCProjectileGrubFX.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCProjectileMetalSheet.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCProjectileTrackingGrub.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCombat.cpp"),
            Object(Matching, "rt/Engine/Game/zCombatAttack.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCombatEffects.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCombatSystem.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCineFX.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCineFXShared.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCineFXTableSceneUT11.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCineFXTableTest.cpp"),
            Object(NonMatching, "rt/Engine/Game/zHud.cpp"),
            Object(NonMatching, "rt/Engine/Game/zContextObject.cpp"),
            Object(NonMatching, "rt/Engine/Game/zContextObjectTightrope.cpp"),
            Object(NonMatching, "rt/Engine/Game/zContextObjectPoleSwing.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCommonPlayerContextActions.cpp"),
            Object(NonMatching, "rt/Engine/Game/zSpringboard.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCarryableObject.cpp"),
            Object(NonMatching, "rt/Engine/Game/zContextObjectClimbableRope.cpp"),
            Object(NonMatching, "rt/Engine/Game/zPlayerActionRopeClimb.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUICustomControllerManager.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUICustomSaveLoad.cpp"),
            Object(NonMatching, "rt/Engine/Game/zParticleSystemPuffer.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCOneLiner.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUICustomConditionallyDisabled.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUICustomMissionCompleteManager.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUICustomOptionsManager.cpp"),
            Object(NonMatching, "rt/Engine/Game/zPlayerActionWallRun.cpp"),
            Object(NonMatching, "rt/Engine/Game/zPlayerActionWallClimb.cpp"),
            Object(NonMatching, "rt/Engine/Game/zHeistManager.cpp"),
            Object(NonMatching, "rt/Engine/Game/zHeistLookoutPoint.cpp"),
            Object(NonMatching, "rt/Engine/Game/zHeistMovementPath.cpp"),
            Object(NonMatching, "rt/Engine/Game/zHeistInteractionObject.cpp"),
            Object(NonMatching, "rt/Engine/Game/zHeistNPCInterface.cpp"),
            Object(NonMatching, "rt/Engine/Game/zHumanVehiclePlayer.cpp"),
            Object(NonMatching, "rt/Engine/Game/zHumanVehiclePlayerActions.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCBase.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCManager.cpp"),
            Object(Matching, "rt/Engine/Game/zNPCTypesIN2.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCommonPlayerActionPushPull.cpp"),
            Object(NonMatching, "rt/Engine/Game/zPushableObject.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCompSteeringBasic.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCChef.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCThiefOld.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCThief.cpp"),
            Object(NonMatching, "rt/Engine/Game/zMiniGameBase.cpp"),
            Object(NonMatching, "rt/Engine/Game/zMiniGameManager.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCSwarmMember.cpp"),
            Object(NonMatching, "rt/Engine/Game/zMiniGamePlayer.cpp"),
            Object(NonMatching, "rt/Engine/Game/zMiniME.cpp"),
            Object(NonMatching, "rt/Engine/Game/zRatBallPlayer.cpp"),
            Object(NonMatching, "rt/Engine/Game/zRatBallPlayerActions.cpp"),
            Object(NonMatching, "rt/Engine/Game/zMiniGameDefendTheBase.cpp"),
            Object(NonMatching, "rt/Engine/Game/zMission.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCAlarmer.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCompStateMachine.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUICustomImageSlideshow.cpp"),
            Object(NonMatching, "rt/Engine/Game/zUICustomCheatsManager.cpp"),
            Object(Matching, "rt/Engine/Game/zCompLogicSwarmMember.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCWaiter.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCompLogicWaiter.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCStirringPlayer.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCStirringRemy.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCompLogicStirringPlayer.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCompLogicStirringRemy.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCompSteeringFlocking.cpp"),
            Object(NonMatching, "rt/Engine/Game/zPlayerLocationPlatform.cpp"),
            Object(NonMatching, "rt/Engine/Game/zMiniGameRatOnBall.cpp"),
            Object(NonMatching, "rt/Engine/Game/zMiniGameRatOnBallPlayer.cpp"),
            Object(NonMatching, "rt/Engine/Game/zMiniGameDefendTheBasePlayer.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCLeftArm.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCRightArm.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCompLogicLeftArm.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCompLogicRightArm.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCSpringBoard.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCompLogicSpringBoard.cpp"),
            Object(Matching, "rt/Engine/Game/zModuleMgr_Registry.cpp"),
            Object(NonMatching, "rt/Engine/Game/zModuleExample.cpp"),
            Object(NonMatching, "rt/Engine/Game/zModuleMgr.cpp"),
            Object(Matching, "rt/Engine/Game/zCompParameters.cpp"),
            Object(NonMatching, "rt/Engine/Game/zMiniGameGrapeStomper.cpp"),
            Object(NonMatching, "rt/Engine/Game/zMiniGameGrapeStomperPlayer.cpp"),
            Object(NonMatching, "rt/Engine/Game/zMiniGamePufferoids.cpp"),
            Object(NonMatching, "rt/Engine/Game/zMiniGamePufferoidsPlayer.cpp"),
            Object(NonMatching, "rt/Engine/Game/zMiniGameCongaLine.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCompLogicPourNSwirl.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCPourNSwirl.cpp"),
            Object(NonMatching, "rt/Engine/Game/zMiniGamePickups.cpp"),
            Object(NonMatching, "rt/Engine/Game/zICompNPCEntity.cpp"),
            Object(NonMatching, "rt/Engine/Game/zHUDCompass.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCContextSensitive.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCompLogicContextSensitive.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCompLogicAnimViewer.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCAnimViewer.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCStates.cpp"),
            Object(NonMatching, "rt/Engine/Game/zMiniGameAvoidTheStuffPlayer.cpp"),
            Object(NonMatching, "rt/Engine/Game/zButtonMasherWidget.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCWalls.cpp"),
            Object(NonMatching, "rt/Engine/Game/zRaftTrack.cpp"),
            Object(Matching, "rt/Engine/Game/zCompSteeringFlockingBrain.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCompLogicSimpleton.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCSimpleton.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCGate.cpp"),
            Object(NonMatching, "rt/Engine/Game/zPourStream.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCompSound.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCompLogicSwarmBug.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCompLogicSwarmOwl.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCSwarmBug.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCSwarmOwl.cpp"),
            Object(NonMatching, "rt/Engine/Game/zComponentTypes.cpp"),
            Object(Matching, "rt/Engine/Game/zDebugTweakBranch.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCompCombat.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCompSteeringGeneric.cpp"),
            Object(NonMatching, "rt/Engine/Game/zNPCInstaKill.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCompPerception.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCompPerceptionParams.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCompLogicInstaKill.cpp"),
            Object(NonMatching, "rt/Engine/Game/zSweptCircle.cpp"),
            Object(NonMatching, "rt/Engine/Game/zCompLogicAlarmer.cpp"),
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
