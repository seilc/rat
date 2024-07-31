#ifndef XMODELBLUR_H
#define XMODELBLUR_H

#include "types.h"

class xModelBlur
{
public:
    struct config_data
    {
        S32 flags;
        F32 radius;
        F32 color_red;
        F32 color_green;
        F32 color_blue;
        F32 color_alpha;
        F32 card_dist;
        F32 zbias;
        S32 max_card_renders;
    };

private:
    struct activity_data;

    activity_data* activity;
};

#endif
