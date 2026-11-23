#ifndef ZINPC_H
#define ZINPC_H

#include "xBase.h"
#include "xEnt.h"
#include "zBase.h"
#include "zICompLogic.h"
#include "zICompNPCEntity.h"
#include "zNPCBase.h"

class zINPC
{
public:
    static bool IsNPCBase(xBase* base) { return base->baseType == eBaseTypeNPC; }

    static xEnt* GetNPCEnt(zNPCBase* base)
    {
#if DEBUG
        xASSERT_FMT_L(53, base->baseType == eBaseTypeNPC, "This function accepts a NPCBase");
#endif
        return base->GetEntityComponent();
    }

    static bool IsNPCEntity(xBase* base) { return base->baseType == eBaseTypeNPCEntity; }
};

#endif
