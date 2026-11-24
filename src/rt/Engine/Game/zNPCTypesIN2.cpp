#include "xCommon.h"
#include "zNPCTypesIN2.h"

#include "xBase.h"
#include "xEntMotion.h"
#include "xPad.h"
#include "xPtankPool.h"
#include "xRand.h"

const zNPCTypeInfoIN2* zNPCTypesGetInfo()
{
    static const zNPCTypeInfoIN2 info[] = {
        { 1, NULL, NULL, NULL, NULL, NULL },
        { 2, NULL, NULL, NULL, NULL, NULL },
        {}
    };

    return info;
}
