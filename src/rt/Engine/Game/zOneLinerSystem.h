#ifndef ZONELINERSYSTEM_H
#define ZONELINERSYSTEM_H

#include "types.h"

struct zPlayer;

// Empty for now - Not found in DWARF
enum zOneLinerTypes
{
};

void zOneLinerSystemSetup();
void zOneLinerSystemReset();
void zOneLinerSystemPlay(zPlayer*, zOneLinerTypes);

#endif
