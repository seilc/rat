#ifndef ZNPCPROJECTILE_H
#define ZNPCPROJECTILE_H

#include "xVec3.h"

struct xEnt;

enum zNPCProjectileType
{
    eProjectileGrub,
    eProjectileTrackingGrub,
    eProjectileMetalSheet,
    eProjectileExplodingGrub
};

class zNPCProjectile
{
public:
    virtual void Freeze() = 0;
    virtual F32 GetRadius() { return 0.0f; }
    virtual bool IsActive() const;

protected:
    xVec3 position;
    bool active;
    zNPCProjectileType type;
};

template <class T, zNPCProjectileType projectileType, S32 maxProjectiles>
class zNPCProjectileBase : public zNPCProjectile
{
public:
    virtual void Reset() {}
    virtual void Update(F32) {}
    virtual void Render() {}
    virtual void PostRender2d() const {}
    virtual void PostRender3d() const {}
    virtual void Explode(bool, const xEnt*) {}
    virtual void Destroy() {}
    virtual bool Setup(void*) { return false; }
    virtual bool LaunchFromEnt(xEnt*, const xVec3*, F32, xEnt*) { return false; }
};

#endif
