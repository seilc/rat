#include "zDebugTweakBranch.h"

#include "xMemMgr.h"

static S32 gDebugTweakBranchMemUsed = 0;

zDebugTweakBranch::zDebugTweakBranch(const char* name, void(*addTweaksFunction)(const char*))
{
    this->addTweaksFunction = addTweaksFunction;
    tweakCB.on_expand = sOnExpand;
    tweakCB.on_collapse = sOnCollapse;
    xTWEAKBRANCH(name, &tweakCB, this, 0x2);
}

zDebugTweakBranch* zDebugTweakBranch::Create(const char* name, void(*addTweaksFunction)(const char*))
{
    gDebugTweakBranchMemUsed += sizeof(zDebugTweakBranch);
    if (gDebugTweakBranchMemUsed >= 10000) {
        return NULL;
    }

    return new (eMemStaticType_0, eMemMgrTag_Debug) zDebugTweakBranch(name, addTweaksFunction);
}

void zDebugTweakBranch::sOnExpand(const tweak_info& info)
{
    zDebugTweakBranch* branch = (zDebugTweakBranch*)info.context;
    branch->OnExpand(info);
}

void zDebugTweakBranch::sOnCollapse(const tweak_info& info)
{
    zDebugTweakBranch* branch = (zDebugTweakBranch*)info.context;
    branch->OnCollapse(info);
}

void zDebugTweakBranch::OnExpand(const tweak_info& info)
{
    if (addTweaksFunction) {
        addTweaksFunction(info.name.text);
    }
    addTweaksFunction = NULL;
}

void zDebugTweakBranch::OnCollapse(const tweak_info&)
{
}
