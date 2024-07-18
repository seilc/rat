#ifndef XCOLOR_H
#define XCOLOR_H

#include "types.h"

#include <rwcore.h>

typedef struct xColor_tag
{
    union
    {
        struct
        {
            U8 r, g, b, a;
        };
        RwRGBA rgba;
    };
} xColor;

typedef struct _xFColor
{
    F32 r, g, b, a;
} xFColor;

extern const xColor g_RED;
extern const xColor g_LIGHT_RED;
extern const xColor g_GREEN;
extern const xColor g_BLUE;
extern const xColor g_DIRTY_BLUE;
extern const xColor g_CYAN;
extern const xColor g_MAGENTA;
extern const xColor g_YELLOW;
extern const xColor g_BLACK;
extern const xColor g_WHITE;
extern const xColor g_GRAY40;
extern const xColor g_GRAY50;
extern const xColor g_GRAY80;
extern const xColor g_CLEAR;
extern const xColor g_NEON_RED;
extern const xColor g_NEON_GREEN;
extern const xColor g_NEON_BLUE;
extern const xColor g_PEACH;
extern const xColor g_FUSCHIA;
extern const xColor g_MAROON;
extern const xColor g_MIDNIGHTBLUE;
extern const xColor g_SEAGREEN;
extern const xColor g_FORESTGREEN;
extern const xColor g_PIMP_GOLD;
extern const xColor g_LIGHT_PIMP_GOLD;
extern const xColor g_ORANGE;
extern const xColor g_KHAKI;
extern const xColor g_LAVENDER;
extern const xColor g_PINK;
extern const xColor g_CHARTREUSE;

#ifdef DEBUG
extern xColor g_COLOR_TRYME;
extern xColor g_COLOR_TRYME2;
extern xColor g_COLOR_TRYME3;
#endif

#endif
