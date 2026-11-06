#ifndef ZSYMBOL_H
#define ZSYMBOL_H

#include "types.h"

struct xModelInstance;
struct xVec3;

namespace z {

struct symbol_map
{
    symbol_map* next;
    U32 id;
    xModelInstance* model;
};

class symbol
{
public:
    static symbol_map* map;

    static void init();
    static void exit(); // UNUSED
    static xModelInstance* find_model_for_symbol(U32 symbol_id); // UNUSED
    static void register_symbol(const char* name, const char* model_name); // UNUSED
    static void render(xVec3 position, const char* symbol_name); // UNUSED
};

}
#endif
