#ifndef ZSYMBOL_H
#define ZSYMBOL_H

#include "types.h"

struct xModelInstance;

namespace z {

struct symbol_map
{
    symbol_map* next;
    U32 id;
    xModelInstance* model;
};

struct symbol
{
    static symbol_map* map;

    static void init();
};

}
#endif
