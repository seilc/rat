#ifndef ZNPCINSTAKILL_H
#define ZNPCINSTAKILL_H

#include "zNPCBase.h"

#if RELEASE
class zCompSteeringGenericParams;
class zCompPerceptionParams;
class zICompNPCEntityParams;
#endif

class zNPCInstaKill : public zNPCBase
{
#if RELEASE
public:
    static zICompNPCEntityParams sEntParams;
    static zCompPerceptionParams sPerceptionParams;
    static zCompSteeringGenericParams sSteeringPatrolParams;
    static zCompSteeringGenericParams sSteeringAlertParams;
    static zCompSteeringGenericParams sSteeringChaseParams;
#endif
};

#endif
