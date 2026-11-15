#ifndef ISYSTEM_H
#define ISYSTEM_H

#include "types.h"

#include <rwcore.h>
#include <dolphin.h>

extern void* hackResourceArenaPointer;
extern GXDrawSyncCallback old_dsc;
extern GXRenderModeObj renderObject;

RwMemoryFunctions* iGetMemoryFunctions(); // UNUSED
S32 iRwResourcesEmptyArena();
void psDebugMessageHandler(RwDebugType, const RwChar* str);
void iVSync();
void my_dsc(U16 token);
void iExceptionMemCrash(const char* location, U32 size, const char* optionalString);
void iSystemRegisterErrorHandlers(); // UNUSED
void iSystemGetWidthHeight(U32& uWidth, U32& uHeight, bool bPALMode);
void iSystemInit(U32);
void iSystemInitGraphics();
void iSystemExit();
#if DEBUG
void debugMallocSave();
void debugMallocDiff();
#endif
void iSystemRWExit();
void iSystemExitRWFailSafe();
void null_func();
S32 DolphinInitMemorySystem(RwMemoryFunctions* mem); // UNUSED
S32 DolphinTermMemorySystem(); // UNUSED
s32 iGetSecond(); // UNUSED
s32 iGetMinute();
s32 iGetHour();
s32 iGetDay();
s32 iGetMonth();
s32 iGetYear(); // UNUSED
u32 iGetCurrFormattedDate(char* p);
u32 iGetCurrFormattedTime(char* p);
void* iSystemARAMAlloc(U32 numbytes_to_use);
void iSystemARAMFree(void* ptr);
void iSystemARAMInit();
U32 iSystemARAMBasePtrForFMOD();
U32 iSystemARAMSizeForFMOD();
void iSystemInitRWFailSafe();
void iSystemAlarmCallBack(OSAlarm* alarm, OSContext*);
void iSystemSleep(S32 milliSecondsToSleep, U32 cancelAlarmsTag);

#endif
