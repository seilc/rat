#ifndef ZSAVELOAD_H
#define ZSAVELOAD_H

#include "types.h"

void zSaveLoadPreAutoSave(bool onOff);
void zSaveLoad_DispatchCB(U32 dispatchEvent, const F32* toParam);
void zSaveLoad_TriggerAutoSave();

#endif
