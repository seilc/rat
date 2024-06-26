#ifndef ZPORTAL_H
#define ZPORTAL_H

#include "xBase.h"
#include "xPortalAsset.h"

typedef struct _zPortal : xBase
{
    xPortalAsset* passet;
} zPortal;

#endif
