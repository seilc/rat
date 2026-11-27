#ifndef ZEGENERATOR_H
#define ZEGENERATOR_H

#include "zEGenAsset.h"
#include "zEnt.h"

struct zLightning;

#define k_MAX_LFX 2

struct zEGenerator : zEnt
{
    zEGenAsset* zasset;
    U16 flags;
    U16 num_dsts;
    F32 tmr;
    xAnimFile* afile;
    xVec3 src_pos;
    xVec3 dst_pos;
    xVec3 dst_off;
    xBase* dst;
    zLightning* lfx[k_MAX_LFX];
};

void zEGenerator_Init(void* egen, void* asset);
void zEGenerator_Init(zEGenerator* egen, xEntAsset* asset);
void zEGenerator_Setup(zEGenerator* egen, xScene*);
void zEGenerator_Save(zEGenerator* ent, xSerial* s);
void zEGenerator_Load(zEGenerator* ent, xSerial* s);
void zEGenerator_Reset(zEGenerator* egen, xScene*);
void zEGenerator_Move(zEGenerator*, xScene*, F32);
void zEGenerator_Update(zEGenerator* egen, xScene* sc, F32 dt);
void zEGenerator_Render(zEGenerator* egen);
void zEGenerator_TurnOn(zEGenerator* egen);
void zEGenerator_TurnOff(zEGenerator* egen);
void zEGenerator_ToggleOn(zEGenerator* egen);
void zEGeneratorEventCB(xBase*, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32);

#endif
