#ifndef XHUDASSET_H
#define XHUDASSET_H

#include "xHud.h"
#include "xDynAsset.h"
#include "xLinkAsset.h"

namespace xhud {

struct asset : xDynAsset
{
    xVec3 loc;
    xVec3 size;
};

struct model_asset : asset
{
    static const U16 VERSION = 1;

    U32 model;

    static const char* type_name() { return "hud:model"; }
};

struct meter_asset : asset
{
    F32 start_value;
    F32 min_value;
    F32 max_value;
    F32 increment_time;
    F32 decrement_time;
    struct
    {
        U32 start_increment;
        U32 increment;
        U32 start_decrement;
        U32 decrement;
    } sound;

    static const char* type_name() { return "hud:meter"; }
};

struct font_meter_asset : meter_asset
{
    static const U16 VERSION = 2;

    font_context font;

    static const char* type_name() { return "hud:meter:font"; }
};

}

#endif
