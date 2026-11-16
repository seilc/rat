#ifndef ZUIUVS_H
#define ZUIUVS_H

#include "types.h"

#include <rwcore.h>
#include <rpworld.h>

F32* zUIUVs_Init(RpAtomic* atomic, U32& texCoordsCount);
void zUIUVs_Update(RpAtomic* atomic, F32 value, const F32* uvCoords, bool, bool update_v, S32 type, F32 offset_u); // UNUSED
void zUIUVs_Reset(RpAtomic* atomic, const F32* uvCoords); // UNUSED
void zUIUVs_Exit(); // UNUSED

#endif
