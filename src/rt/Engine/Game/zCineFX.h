#ifndef ZCINEFX_H
#define ZCINEFX_H

#include "types.h"

#define CINFX_FRAMES(n) ((n) * (1.0f / 30.0f))

struct NCINEntry;

struct NCINMap
{
    char* cinname;
    NCINEntry* cinfxtab;
    U32 cinhash;
};

void zCineFXRegisterTable(NCINMap* maplist);

#endif
