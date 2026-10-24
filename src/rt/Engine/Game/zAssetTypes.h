#ifndef ZASSETTYPES_H
#define ZASSETTYPES_H

#include "types.h"

void* Spline_Read(void*, U32, void* indata, U32 insize, U32* outsize);
void* RWTX_Read(void*, U32, void* indata, U32 insize, U32* outsize);
void TextureRW3_Unload(void* userdata, U32);

#endif
