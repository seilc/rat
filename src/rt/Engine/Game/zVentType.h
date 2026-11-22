#ifndef ZVENTTYPE_H
#define ZVENTTYPE_H

#include "xBase.h"
#include "xDynAsset.h"
#include "xParEmitter.h"

struct zVentType : xBase
{
    struct asset_type : xDynAsset
    {
        enum { VERSION = 1 };

        static const char* type_name() { return "game_object:VentType"; }

        U32 idleEmitter;
        U32 idleSound;
        U32 warnEmitter;
        U32 warnSound;
        U32 damageEmitter;
        U32 damageSound;
    };

    const asset_type* asset;
    xParEmitter* idleEmitter;
    xParEmitter* warnEmitter;
    xParEmitter* damageEmitter;

    void load(const asset_type& a);

    static void load(xBase& data, xDynAsset& asset, size_t asset_size);
};

#endif
