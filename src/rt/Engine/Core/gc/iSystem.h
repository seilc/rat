#ifndef ISYSTEM_H
#define ISYSTEM_H

#include "types.h"

#include <dolphin.h>

extern GXRenderModeObj renderObject;

void iExceptionMemCrash(const char* location, U32 size, const char* optionalString);
void iSystemSleep(S32 milliSecondsToSleep, U32 cancelAlarmsTag);

#endif
