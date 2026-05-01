#ifndef ZDEBUGTWEAKBRANCH_H
#define ZDEBUGTWEAKBRANCH_H

#include "xDebugTweak.h"

class zDebugTweakBranch
{
private:
    tweak_callback tweakCB;
    void(*addTweaksFunction)(const char*);

public:
    zDebugTweakBranch(const char* name, void(*addTweaksFunction)(const char*));
    static zDebugTweakBranch* Create(const char* name, void(*addTweaksFunction)(const char*));
    static void sOnExpand(const tweak_info& info);
    static void sOnCollapse(const tweak_info& info);
    void OnExpand(const tweak_info& info);
    void OnCollapse(const tweak_info&);
};

#endif
