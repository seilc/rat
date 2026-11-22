#ifndef ZMINIGAMECONGALINE_H
#define ZMINIGAMECONGALINE_H

#include "zMiniGameBase.h"

struct zMiniGameAssetCongaLine : zMiniGameAsset
{
    enum { VERSION = 1 };

    U32 firstStageID;
};

class zMiniGameCongaLine : public zMiniGameBase
{
public:
    zMiniGameCongaLine(zMiniGameAsset* a);

    void CreatePlayers(xDynAsset*); // UNUSED
    static void Init(xBase& data, xDynAsset& asset, size_t asset_size);
    inline zMiniGameAssetCongaLine* GetAsset() const; // UNUSED

    virtual void SceneSetup();
    virtual void Update(F32);
    virtual void Render();
    virtual void Reset();
#if DEBUG || RELEASE
    virtual void AddTweaks(const char*);
#endif
};

#endif
