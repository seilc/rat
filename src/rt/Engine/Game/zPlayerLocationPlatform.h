#ifndef ZPLAYERLOCATIONPLATFORM_H
#define ZPLAYERLOCATIONPLATFORM_H

#include "xBase.h"
#include "zPlayerLocationPlatformConnectorAsset.h"

struct xEnt;

class zPlayerLocationPlatformConnector : public xBase
{
public:
    zPlayerLocationPlatformConnector(zPlayerLocationPlatformConnectorAsset* theAsset);

    static void Init(xBase& data, xDynAsset& asset, size_t asset_size);
    void Update(F32);
    static void UpdateAll(F32 dt);
    static void Exit();

    static zPlayerLocationPlatformConnector* head;

protected:
    zPlayerLocationPlatformConnectorAsset* asset;
    bool firstUpdate;
    xEnt* platform;
    xEnt* attachEnt;
    zPlayerLocationPlatformConnector* next;
};

#endif
