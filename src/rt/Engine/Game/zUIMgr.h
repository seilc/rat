#ifndef ZUIMGR_H
#define ZUIMGR_H

#include "types.h"

class zUI;

void zUISetTimer(F32 value);
void zUIMgrSetScreenLetterBox(bool value);
bool zUIMgrGetScreenLetterBox();
void zUIMgrEnableHDR(bool enabled);
void zUIMgrSortList(zUI** list, S32 count);

#endif
