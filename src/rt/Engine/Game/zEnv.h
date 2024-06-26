#ifndef ZENV_H
#define ZENV_H

#include "xBase.h"
#include "xEnvAsset.h"

typedef struct _zEnv : xBase
{
    xEnvAsset* easset;
} zEnv;

#endif
