#include "xFX.h"

#include "decomp.h"

DECOMP_FORCEBLOCK((xFXRibbon* r) {
    for (xFXRibbon::joint_iterator it = r->begin(); it != r->end(); ++it) {}
})
