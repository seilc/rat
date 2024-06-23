#ifndef XCOLOR_H
#define XCOLOR_H

#include "types.h"

#include <rwcore.h>

typedef union xColor_tag
{
    struct
    {
        U8 r, g, b, a;
    };
    RwRGBA rgba;
} xColor;

extern const xColor_tag g_RED;
extern const xColor_tag g_LIGHT_RED;
extern const xColor_tag g_GREEN;
extern const xColor_tag g_BLUE;
extern const xColor_tag g_DIRTY_BLUE;
extern const xColor_tag g_CYAN;
extern const xColor_tag g_MAGENTA;
extern const xColor_tag g_YELLOW;
extern const xColor_tag g_BLACK;
extern const xColor_tag g_WHITE;
extern const xColor_tag g_GRAY40;
extern const xColor_tag g_GRAY50;
extern const xColor_tag g_GRAY80;
extern const xColor_tag g_CLEAR;
extern const xColor_tag g_NEON_RED;
extern const xColor_tag g_NEON_GREEN;
extern const xColor_tag g_NEON_BLUE;
extern const xColor_tag g_PEACH;
extern const xColor_tag g_FUSCHIA;
extern const xColor_tag g_MAROON;
extern const xColor_tag g_MIDNIGHTBLUE;
extern const xColor_tag g_SEAGREEN;
extern const xColor_tag g_FORESTGREEN;
extern const xColor_tag g_PIMP_GOLD;
extern const xColor_tag g_LIGHT_PIMP_GOLD;
extern const xColor_tag g_ORANGE;
extern const xColor_tag g_KHAKI;
extern const xColor_tag g_LAVENDER;
extern const xColor_tag g_PINK;
extern const xColor_tag g_CHARTREUSE;

extern xColor_tag g_COLOR_TRYME;
extern xColor_tag g_COLOR_TRYME2;
extern xColor_tag g_COLOR_TRYME3;

#endif