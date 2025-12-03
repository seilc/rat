#ifndef ZNPCSQUAD_H
#define ZNPCSQUAD_H

#include "types.h"

class zNPCCommon;

class zNPCSquad
{
public:
    zNPCSquad();
    virtual ~zNPCSquad();
    virtual void Update(F32) = 0;

protected:
    S32 totalMembers;
    zNPCCommon* member[16];
};

class zNPCSquadMeleeAttack : private zNPCSquad
{
public:
    zNPCSquadMeleeAttack();
    virtual ~zNPCSquadMeleeAttack();
    virtual void Update(F32);
};

#endif
