#ifndef ZGRID_H
#define ZGRID_H

#include "xGrid.h"

struct zScene;

extern xGrid colls_grid;
extern xGrid colls_oso_grid;
extern xGrid npcs_grid;
extern xGrid grabbable_grid;

void zGridReset(zScene* s);
void zGridInit(zScene* s);
void zGridExit(zScene*);
void zGridUpdateEnt(xEnt* ent);

#endif
