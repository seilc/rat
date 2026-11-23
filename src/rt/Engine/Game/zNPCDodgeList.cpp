#include "xCommon.h"
#include "zNPCDodgeList.h"

#include "containers.h"
#include "iMath.h"
#include "xCam.h"
#include "xDebugTweak.h"
#include "xEnt.h"
#include "xEntMotion.h"
#include "xFont.h"
#include "xHud.h"
#include "xMath3.h"
#include "xMemMgr.h"
#include "xMemory.h"
#include "xPtankPool.h"
#include "xRand.h"
#include "zPlayerContainer.h"

#define MAX_DODGE_LIST_OBJECTS 40

struct zNPCDodgeListObject
{
    const xEnt* target;
    const xEnt* flyingObject;
    xMat4x3 oldMat;
};

xTWEAKABLE F32 ZNPC_DODGE_LIST_CONE_INTERSECT_ANGLE = 30.0f;
xTWEAKABLE F32 ZNPC_DODGE_LIST_CONE_HEIGHT_SQ_MULTIPLIER = 1.3f;
xTWEAKABLE F32 ZNPC_DODGE_LIST_FROZONE_ICE_BLAST_RANGE_MULTIPLIER = 1.3f;
xTWEAKABLE bool ZNPC_DODGE_LIST_ENABLE_DODGE = true;
xTWEAKABLE bool ZNPC_DODGE_LIST_DRAW_CONE = false;

static zNPCDodgeListObject* sNPCDodgeListObjects;
static xVec3* sNPCDodgeListObjectsVelocity;
static xVec3* sNPCDodgeListObjectsVelocityNormalized;
static xVec3* sNPCDodgeListObjectsLocation;
static F32 sSinAngle;
static F32 sCos2Angle;

static F32 sNPCDodgeListGetCos2BetweenVectors(const xVec3& vec1, const xVec3& vec2, F32& a2b2)
{
    F32 dotVec1Vec2, sqVec1 = xVec3Length2(&vec1);
    F32 sqVec2 = xVec3Length2(&vec2);
    a2b2 = sqVec1 * sqVec2;
    dotVec1Vec2 = xVec3Dot(&vec1, &vec2);
    if (a2b2 == 0.0f) {
        return 2.0f;
    }

    if (dotVec1Vec2 < 0.0f) {
        return -xsqr(dotVec1Vec2);
    }

    return xsqr(dotVec1Vec2);
}

static inline F32 sNPCDodgeListCrossXZ(const xVec3& a, const xVec3& b)
{
    return a.z * b.x - b.z * a.x;
}

#if DEBUG || RELEASE
static void sNPCDodgeListAddTweaks()
{
    xTWEAK("NPC OLD|debug|Dodge|Dodge Cone Angle", &ZNPC_DODGE_LIST_CONE_INTERSECT_ANGLE, 0.0f, 180.0f, NULL, NULL, 0);
    xTWEAK("NPC OLD|debug|Dodge|Cone Height^2 Multiplier", &ZNPC_DODGE_LIST_CONE_HEIGHT_SQ_MULTIPLIER, 0.0f, 10.0f, NULL, NULL, 0);
    xTWEAK_BOOL("NPC OLD|debug|Dodge|Draw Dodge Cone", &ZNPC_DODGE_LIST_DRAW_CONE, NULL, NULL, 0);
    xTWEAK("NPC OLD|debug|Dodge|Frozone Ice Blast Cone Multiplier", &ZNPC_DODGE_LIST_FROZONE_ICE_BLAST_RANGE_MULTIPLIER, 0.0f, 10.0f, NULL, NULL, 0);
    xTWEAK_BOOL("NPC OLD|debug|Dodge|Enable Dodging", &ZNPC_DODGE_LIST_ENABLE_DODGE, NULL, NULL, 0);
}
#endif

void zNPCDodgeListInit()
{
    sNPCDodgeListObjects = (zNPCDodgeListObject*)xMEMALLOC_L(MAX_DODGE_LIST_OBJECTS * sizeof(zNPCDodgeListObject), 0, eMemMgrTag_Enemy, 0, 94);
    sNPCDodgeListObjectsVelocity = (xVec3*)xMEMALLOC_L(MAX_DODGE_LIST_OBJECTS * sizeof(xVec3), 0, eMemMgrTag_Enemy, 0, 95);
    sNPCDodgeListObjectsVelocityNormalized = (xVec3*)xMEMALLOC_L(MAX_DODGE_LIST_OBJECTS * sizeof(xVec3), 0, eMemMgrTag_Enemy, 0, 96);
    sNPCDodgeListObjectsLocation = (xVec3*)xMEMALLOC_L(MAX_DODGE_LIST_OBJECTS * sizeof(xVec3), 0, eMemMgrTag_Enemy, 0, 97);

    sSinAngle = isin((PI / 180.0f) * (ZNPC_DODGE_LIST_CONE_INTERSECT_ANGLE / 2.0f));
    sCos2Angle = 1.0f - xsqr(sSinAngle);

    zNPCDodgeListReset();

#if DEBUG || RELEASE
    sNPCDodgeListAddTweaks();
#endif
}

void zNPCDodgeListReset()
{
    for (S32 i = 0; i < MAX_DODGE_LIST_OBJECTS; i++) {
        sNPCDodgeListObjects[i].target = NULL;
        sNPCDodgeListObjects[i].flyingObject = NULL;
    }
}

U32 zNPCDodgeListGetDodgeDirection(xEnt* ent, xVec3& poi)
{
    if (!ZNPC_DODGE_LIST_ENABLE_DODGE) {
        return 0;
    }

    for (S32 iter = 0; iter < MAX_DODGE_LIST_OBJECTS; iter++) {
        if (sNPCDodgeListObjects[iter].flyingObject) {
            xVec3 projToNPC;
            F32 checkDist2, cos2, a2b2;
            xVec3 sphereIsectConeOrigin;

            xVec3Sub(projToNPC, ent->frame->mat.pos, sNPCDodgeListObjectsLocation[iter]);
            if (xVec3Dot(&projToNPC, &ent->frame->mat.at) > 0.0f) {
                return 0;
            }

            xVec3SMul(sphereIsectConeOrigin, sNPCDodgeListObjectsVelocityNormalized[iter], -ent->bound.sph.r / sSinAngle);
            xVec3Add(sphereIsectConeOrigin, sNPCDodgeListObjectsLocation[iter]);
            xVec3Sub(projToNPC, ent->frame->mat.pos, sphereIsectConeOrigin);

            cos2 = sNPCDodgeListGetCos2BetweenVectors(sNPCDodgeListObjectsVelocity[iter], projToNPC, a2b2);
            if (cos2 == 2.0f) {
                return 0;
            }

            if (cos2 > sCos2Angle * a2b2) {
                checkDist2 = xVec3Dist2(&sNPCDodgeListObjectsLocation[iter], &ent->frame->mat.pos);
                if (ZNPC_DODGE_LIST_CONE_HEIGHT_SQ_MULTIPLIER * xVec3Length2(&sNPCDodgeListObjectsVelocity[iter]) > checkDist2) {
                    xVec3Copy(&poi, &sNPCDodgeListObjectsLocation[iter]);
                    if (sNPCDodgeListCrossXZ(sNPCDodgeListObjectsVelocity[iter], projToNPC) < 0.0f) {
                        return 2;
                    } else if (sNPCDodgeListCrossXZ(sNPCDodgeListObjectsVelocity[iter], projToNPC) > 0.0f) {
                        return 1;
                    } else if (xrand_RandomInt32() & 1) {
                        return 1;
                    } else {
                        return 2;
                    }
                }
            }
        }
    }

    return 0;
}

// UNUSED: needed to match (code from RotU GC)
S32 zNPCDodgeListAdd(xEnt* inFlyingObject, xEnt* inTarget)
{
    xASSERT(inFlyingObject);

    for (S32 i = 8; i < MAX_DODGE_LIST_OBJECTS; i++) {
        if (!sNPCDodgeListObjects[i].flyingObject) {
            sNPCDodgeListObjects[i].flyingObject = inFlyingObject;
            sNPCDodgeListObjects[i].target = inTarget;
            sNPCDodgeListObjects[i].oldMat = inFlyingObject->frame->oldmat;
            return i;
        }
    }

    return -1;
}

void zNPCDodgeListUpdate(F32)
{
}
