#ifndef ZNPCSQUAD_H
#define ZNPCSQUAD_H

#include "types.h"

namespace zNPC {
class zNPCCommon;
}

class zNPCSquad
{
public:
    zNPCSquad();
    virtual ~zNPCSquad();
    virtual void Update(F32) = 0;
    void Reset(); // UNUSED
    U8 Add(zNPC::zNPCCommon* addMember); // UNUSED
    U8 Delete(zNPC::zNPCCommon* delMember); // UNUSED
    U8 IsMember(zNPC::zNPCCommon* isMember); // UNUSED

protected:
    S32 totalMembers;
    zNPC::zNPCCommon* member[16];
};

class zNPCSquadMeleeAttack : private zNPCSquad
{
public:
    zNPCSquadMeleeAttack();
    virtual ~zNPCSquadMeleeAttack();
    virtual void Update(F32);
};

#endif
