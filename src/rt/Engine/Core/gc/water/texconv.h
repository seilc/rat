#ifndef TEXCONV_H
#define TEXCONV_H

#include "types.h"

#include <rwcore.h>

RwTexture* TextureReadDiffBumpRGBA8(RwChar* name); // UNUSED
RwTexture* TextureReadRGBA8(RwChar* name, RwChar* maskName); // UNUSED
RwTexture* TextureReadIA8(RwChar* name, RwChar* maskName); // UNUSED
RwTexture* TextureDiffBumpRGBA8(RwTexture* tex, F32 range_scale);
RwTexture* TextureDiffBumpRGBA8(RwTexture* tex, RwTexture* bump);

#endif
