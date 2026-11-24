#ifndef ZANIMLIST_H
#define ZANIMLIST_H

#include "types.h"

struct xAnimTable;

void zAnimListInit();
void zAnimListExit();
xAnimTable* zAnimListGetTable(U32 id);
S32 zAnimListGetNumUsed(U32 id);

#endif
