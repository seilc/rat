#ifndef ZPARTICLECUSTOM_H
#define ZPARTICLECUSTOM_H

#include "xVec3.h"

F32 zParticleCustomRequestGlint(F32 amount, S32 systemID);
void zParticleCustomEmitGlint(const xVec3* loc, const xVec3* vel, const F32* length_vel, S32 amount, F32 life_min,
                              F32 life_range, F32 width_min, F32 width_range, F32 length_min, F32 length_range,
                              F32 length_clamp_min, F32 length_clamp_range, F32 dt, F32 gravity, S32 systemID);
void zParticleCustomCreateGlintSystem(S32& systemID, U32 textureID, const char* debugName);

#endif
