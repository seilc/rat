#include "xCommon.h"
#include "zSymbol.h"

#include "containers.h"
#include "xBase.h"
#include "xCam.h"
#include "zPlayerContainer.h"

namespace z {

symbol_map* symbol::map = NULL;

void symbol::init()
{
    map = NULL;
}

}
