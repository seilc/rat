#include "xFX.h"

#ifndef NON_MATCHING
static void __unused(xFXRibbon* r)
{
    for (xFXRibbon::joint_iterator it = r->begin(); it != r->end(); ++it) {}
}
#endif
