#ifndef IMODEL_H
#define IMODEL_H

#include "types.h"

#include <rwcore.h>
#include <rpworld.h>

U32 iModelNumBones(RpAtomic* model);
void iModelRender(RpAtomic* model, RwMatrix* mat);

#endif
