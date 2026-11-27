#ifndef ZNPCPROJECTILEFROZEN_H
#define ZNPCPROJECTILEFROZEN_H

struct xMat4x3;
struct xVec3;

enum zNPCProjectileFrozenType
{
    ezNPCProjectileFrozenType_Grub,
    ezNPCProjectileFrozenType_TrackingGrub,
    ezNPCProjectileFrozenType_Count,
    ezNPCProjectileFrozenType_Invalid = -1
};

void zNPCProjectileFreeze(const xMat4x3& frameMatrix, const xVec3& velocity, zNPCProjectileFrozenType type);

#endif
