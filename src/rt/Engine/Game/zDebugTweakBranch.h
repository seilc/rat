#ifndef ZDEBUGTWEAKBRANCH_H
#define ZDEBUGTWEAKBRANCH_H

#include "xDebugTweak.h"

typedef void(*zDebugTweakBranchCallback)(const char*);

class zDebugTweakBranch
{
private:
    tweak_callback tweakCB;
    zDebugTweakBranchCallback addTweaksFunction;

public:
    zDebugTweakBranch(const char* name, zDebugTweakBranchCallback addTweaksFunction);
    static zDebugTweakBranch* Create(const char* name, zDebugTweakBranchCallback addTweaksFunction);
    static void sOnExpand(const tweak_info& info);
    static void sOnCollapse(const tweak_info& info);
    void OnExpand(const tweak_info& info);
    void OnCollapse(const tweak_info&);
};

#endif
