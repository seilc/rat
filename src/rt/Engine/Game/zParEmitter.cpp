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
    zParEmitter* pe;
    S32 i = s->baseCount[eBaseTypeParticleEmitter];
    
    for (pe = (zParEmitter*)s->baseList[eBaseTypeParticleEmitter]; i > 0; pe++, i--) {
        if (pe->tasset->id == asset_id) {
            return pe;
        }
    }

    return NULL;
}

zParEmitter* zParEmitterFind(const char* asset_name)
{
    U32 asset_id = xStrHash(asset_name);
    return zParEmitterFind(asset_id);
}
