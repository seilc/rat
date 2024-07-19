#include "xHierarchyBound.h"

#include "xDebugTweak.h"
#include "xDraw.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef DEBUGRELEASE
namespace {

    U32 CountElements(const char** list)
    {
        const char** it = list;
        while (*it) it++;
        return it - list;
    }

    S32 GetNumberFromString(const char* string, size_t size)
    {
        const char* end = string + size;
        const char* start = end - 1;
        while (isdigit(*start)) start--;
        start++;

        char buffer[32];
        memcpy(buffer, start, end - start);
        buffer[end-start] = '\0';

        return atoi(buffer);
    }

    void OnHierarchyRootSelect(const tweak_info& info)
    {
        xHierarchyBound* bound = (xHierarchyBound*)info.context;
        bound->tweakInfo.rootSelected = true;
    }

    void OnHierarchyRootUnselect(const tweak_info& info)
    {
        xHierarchyBound* bound = (xHierarchyBound*)info.context;
        bound->tweakInfo.rootSelected = false;
    }

    void OnHierarchySelect(const tweak_info& info)
    {
        xHierarchyBound* bound = (xHierarchyBound*)info.context;
        bound->tweakInfo.hilight = GetNumberFromString(info.name.text, info.name.size);
    }

    void OnHierarchyUnselect(const tweak_info& info)
    {
        xHierarchyBound* bound = (xHierarchyBound*)info.context;
        bound->tweakInfo.hilight = -1;
    }

    void AddHierarchyTweak(const char* base, xHierarchyBound* bound, U32 which)
    {
        static const tweak_callback tweak_param_cb = {
            NULL,
            OnHierarchySelect,
            OnHierarchyUnselect,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL
        };

        char buffer[128];

        sprintf(buffer, "%s|Entry %2d", base, which);
        xTWEAKBRANCH(buffer, &tweak_param_cb, bound, 0x2);

        sprintf(buffer, "%s|Entry %2d|Bone", base, which);
        if (bound->tweakInfo.boneNames) {
            U32 boneNameCount = CountElements(bound->tweakInfo.boneNames);
            xTWEAKSELECT(buffer, &bound->nodes[which].bone, bound->tweakInfo.boneNames, NULL, boneNameCount, NULL, NULL, 0x2);
        } else {
            xTWEAK(buffer, &bound->nodes[which].bone, 0, bound->tweakInfo.boneCount, NULL, NULL, 0x2);
        }

        sprintf(buffer, "%s|Entry %2d|Radius", base, which);
        xTWEAK(buffer, &bound->nodes[which].sphere.r, 0.0001f, 10000.0f, NULL, NULL, 0x2);

        sprintf(buffer, "%s|Entry %2d|User Data", base, which);
        xTWEAK(buffer, &bound->nodes[which].userData, 0, 0xFFFF, NULL, NULL, 0x2);
    }

    void OnHierachyChangeCount(const tweak_info& info)
    {
        xHierarchyBound* bound = (xHierarchyBound*)info.context;

        char baseName[128];

        const char* nameEnd = info.name.text + info.name.size - 1;
        while (true) {
            if (*nameEnd == '|') break;
            nameEnd--;
        }

        memcpy(baseName, info.name.text, nameEnd - info.name.text);
        baseName[nameEnd-info.name.text] = '\0';

        if (bound->count < bound->tweakInfo.lastCount) {
            for (U32 i = bound->count; i < bound->tweakInfo.lastCount; i++) {
                char buffer[128];
                sprintf(buffer, "%s|Entry %2d", baseName, i);
                xTWEAKREMOVE(buffer);
            }
        } else {
            for (U32 i = bound->tweakInfo.lastCount; i < bound->count; i++) {
                AddHierarchyTweak(baseName, bound, i);
            }
        }

        bound->tweakInfo.lastCount = bound->count;
    }

}
#endif

S32 xHierarchyBoundSetup(xHierarchyBound* bound, const xHierarchyBoundInitData* init, U32 extraBounds)
{
    U32 nodesSize = 0;
    while (init[nodesSize].bone >= -1) nodesSize++;
    nodesSize += extraBounds;
    xASSERT(202, nodesSize <= 255);

    bound->nodes = (xHierarchyNode*)xMEMALLOC(sizeof(xHierarchyNode) * nodesSize, 0, eMemMgrTag_HierarchyBound, 0, 205);
    bound->maxCount = nodesSize;

    return sizeof(xHierarchyNode) * nodesSize;
}

S32 xHierarchyBoundSetup(xHierarchyBound* bound, U32 nodesNumber)
{
    U32 nodesSize = nodesNumber;
    xASSERT(215, nodesSize <= 255);

    bound->nodes = (xHierarchyNode*)xMEMALLOC(sizeof(xHierarchyNode) * nodesSize, 0, eMemMgrTag_HierarchyBound, 0, 218);
    bound->maxCount = nodesSize;

    return sizeof(xHierarchyNode) * nodesSize;
}

void xHierarchyBoundInit(xHierarchyBound* bound, const xHierarchyBoundInitData* init, F32 masterRadius, F32 scale)
{
    xASSERT(227, bound->nodes);

    U32 count = 0;
    while (init[count].bone >= -1) count++;
    bound->count = count;

    for (U32 i = 0; i < count; i++) {
        xHierarchyNode& node = bound->nodes[i];
        node.bone = init[i].bone;
        memset(&node.sphere.center, 0, sizeof(node.sphere.center));
        node.sphere.r = scale * init[i].radius;
        node.userData = init[i].userData;
    }

    xASSERT(246, count <= 255);
    
    U32 extraBounds = bound->maxCount - count;
    memset(bound->nodes + count, 0, sizeof(xHierarchyNode) * extraBounds);

    memset(&bound->master.center, 0, sizeof(bound->master.center));
    bound->master.r = masterRadius * scale;
}

void xHierarchyBoundDraw(const xHierarchyBound* bound, U8 mR, U8 mG, U8 mB, U8 mA, U8 sR, U8 sG, U8 sB, U8 sA, S8 hilight, U8 hR, U8 hG, U8 hB, U8 hA)
{
    xDrawSetColor(mR, mG, mB, mA);
    xDrawSphere(&bound->master, 0xC0006);

    xDrawSetColor(sR, sG, sB, sA);
    for (U32 i = 0; i < bound->count; i++) {
        if ((S32)i == hilight) {
            xDrawSetColor(hR, hG, hB, hA);
        }
        xDrawSphere(&bound->nodes[i].sphere, 0xC0006);
        if ((S32)i == hilight) {
            xDrawSetColor(sR, sG, sB, sA);
        }
    }
}

bool xHierarchyBoundTest(const xHierarchyBound* bound, xSweptSphere* sphere, U16* boundUserData, S8* subsphere, bool skipMasterCheck)
{
    if (!skipMasterCheck && !xSweptSphereToSphere(sphere, &bound->master)) {
        return false;
    }

    xSweptSphereResetCollision(sphere);

    for (U32 i = 0; i < bound->count; i++) {
        if (xSweptSphereToSphere(sphere, &bound->nodes[i].sphere)) {
            if (boundUserData) {
                *boundUserData = bound->nodes[i].userData;
            }
            if (subsphere) {
                *subsphere = i;
            }
            return true;
        }
    }

    return false;
}

#ifdef DEBUGRELEASE
void xHierarchyBoundAddDebugTweaks(xHierarchyBound* bound, const char* base, const char** boneNames, S32 boneCount)
{
    static const tweak_callback master_tweaks = {
        NULL,
        OnHierarchyRootSelect,
        OnHierarchyRootUnselect,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    };

    static const tweak_callback entry_tweaks = {
        OnHierachyChangeCount,
        OnHierarchyRootSelect,
        OnHierarchyRootUnselect,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    };

    xASSERT(461, boneNames != NULL || boneCount > 0);
    xASSERT(462, boneCount < 64);

    char buffer[128];

    sprintf(buffer, "%s|\1Master Radius", base);
    xTWEAK(buffer, &bound->master.r, 0.0001f, 10000.0f, &master_tweaks, bound, 0x2);

    sprintf(buffer, "%s|\1Entry Count", base);
    xTWEAK(buffer, &bound->count, 1, bound->maxCount, &entry_tweaks, bound, 0x2);

    sprintf(buffer, "%s|\1Entry Max", base);
    xTWEAK(buffer, &bound->maxCount, 0, 0, &master_tweaks, bound, 0x3);

    bound->tweakInfo.boneNames = boneNames;
    bound->tweakInfo.boneCount = boneCount;
    bound->tweakInfo.hilight = -1;
    bound->tweakInfo.lastCount = bound->count;
    bound->tweakInfo.rootSelected = false;

    for (U32 i = 0; i < bound->count; i++) {
        AddHierarchyTweak(base, bound, i);
    }
}

#ifndef NON_MATCHING
static void __unused(U32 count, U32 extraSpheres)
{
    xASSERT(0, count+extraSpheres <= 255);

    xTWEAK("%s|\1Count", (F32*)0, 0, 0, 0, 0, 0);
    xTWEAK("%s|\1Max", (F32*)0, 0, 0, 0, 0, 0);
}
#endif
#endif
