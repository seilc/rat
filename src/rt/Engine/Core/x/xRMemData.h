#ifndef XRMEMDATA_H
#define XRMEMDATA_H

#include "xBase.h"

struct RyzMemGrow
{
    S32 IsEnabled() { return flg_grow & 0x1; }

protected:
    S32 flg_grow;
    S32 amt;
    char* ptr;
    xBase* user;
    S32 amt_last;
    char* ptr_last;
    xBase* user_last;
};

#endif