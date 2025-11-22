#include "zSymbol.h"

namespace z {

symbol_map* symbol::map = NULL;

void symbol::init()
{
    map = NULL;
}

}
