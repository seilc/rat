#ifndef ITIME_H
#define ITIME_H

#include "types.h"

#include <dolphin.h>

typedef OSTime iTime;

#define iTimeFromSec(sec) ((iTime)OSSecondsToTicks(sec))

void iTimeInit();
void iTimeExit();
iTime iTimeGet();
F32 iTimeDiffSec(iTime time);
F32 iTimeDiffSec(iTime t0, iTime t1);
F32 iTimeGetGame();
void iTimeGameAdvance(F32 elapsed);
void iTimeSetGame(F32 time);
void iProfileSetBuffer(void*, U32); // UNUSED
void iProfileEnable(); // UNUSED
void iProfileDisable(); // UNUSED
void iProfileClear(U32 sceneID);
void iProfileDump(); // UNUSED
void iFuncProfileDump();
void iFuncProfileFuncs(S32, S32, F32);
void iInstrProfileDump(char*);
void iFuncProfileParse(char*, S32); // UNUSED
void iFuncMasterLoad(const char* filename); // UNUSED
S32 iProfGetFuncAtAddress(U32 address, char** func, char** file); // UNUSED
void iProfDumpStackTrace(U32* stacktrace, U32 maxdepth); // UNUSED

#endif
