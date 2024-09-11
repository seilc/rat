#ifndef ISCRFX_H
#define ISCRFX_H

#include "types.h"

#include <rwcore.h>

void iScrFxCameraCreated(RwCamera* pCamera);
void iScrFxCameraEndScene(RwCamera* pCamera);
S32 iScrFxCameraDestroyed(RwCamera*);

#endif
