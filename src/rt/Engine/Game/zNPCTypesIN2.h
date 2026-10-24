#ifndef ZNPCTYPESIN2_H
#define ZNPCTYPESIN2_H

#include "types.h"

typedef void(*zNPCTypeInfoCallback)();

struct zNPCTypeInfoIN2
{
    U32 id;
    zNPCTypeInfoCallback scene_init;
    zNPCTypeInfoCallback scene_enter;
    zNPCTypeInfoCallback setup;
    zNPCTypeInfoCallback reset;
    zNPCTypeInfoCallback scene_exit;
};

const zNPCTypeInfoIN2* zNPCTypesGetInfo();

#endif
