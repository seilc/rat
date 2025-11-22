#ifndef ZNPCTYPESIN2_H
#define ZNPCTYPESIN2_H

#include "types.h"

struct zNPCTypeInfoIN2
{
    U32 id;
    void(*scene_init)();
    void(*scene_enter)();
    void(*setup)();
    void(*reset)();
    void(*scene_exit)();
};

const zNPCTypeInfoIN2* zNPCTypesGetInfo();

#endif
