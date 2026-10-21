#include "zParEmitter.h"

#include "zGlobals.h"
#include "zScene.h"

zParEmitter* zParEmitterFind(U32 asset_id)
{
    zScene* s = globals.sceneCur;
    if (!s) {
        return NULL;
    }

    S32 total;
    (void)total;
    (void)total;
    zParEmitter* pe;
    S32 i = s->baseCount[eBaseTypeParticleEmitter];
    pe = (zParEmitter*)s->baseList[eBaseTypeParticleEmitter];
    for (; i > 0; i--) {
        if (pe->tasset->id == asset_id) {
            return pe;
        }
        pe++;
    }

    return NULL;
}

zParEmitter* zParEmitterFind(const char* asset_name)
{
    U32 asset_id = xStrHash(asset_name);
    return zParEmitterFind(asset_id);
}
