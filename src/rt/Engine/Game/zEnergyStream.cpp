#include "xCommon.h"
#include "xFX.h"

#include "xBase.h"
#include "xCam.h"
#include "xEnt.h"
#include "xPtankPool.h"
#include "zPlayerContainer.h"

#include "decomp.h"

DECOMP_FORCEBLOCK((xFXRibbon* r) {
    for (xFXRibbon::joint_iterator it = r->begin(); it != r->end(); ++it) {}
})
