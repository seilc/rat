#include "xCommon.h"
#include "zCompLogicStirringPlayer.h"

#include "xDebugTweak.h"
#include "xEvent.h"
#include "xFont.h"
#include "xPad.h"
#include "xString.h"
#include "zEvent.h"
#include "zHashes.h"
#include "zICompNPCEntity.h"
#include "zMiniME.h"
#include "zScene.h"

struct rgba_node
{
    F32 time;
    F32 r;
    F32 g;
    F32 b;
    F32 a;
};

void zCompLogicStirringPlayer::CreateAnimTable(xAnimTable* table)
{
    xAnimTableNewState(table, "STIRRINGPLAYER_L_MOVE01", AnimState_Loop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);

    zICompLogic::CreateAnimTable(table);
}

namespace
{
    xVec3 potPositions[4] = {
        { -1.72f, 1.636f, -0.32f },
        { -1.72f, 1.636f, 0.354f },
        { -2.286f, 1.636f, 0.354f },
        { -2.286f, 1.636f, -0.32f }
    };

    xTWEAKABLE F32 sPotBoilStart = 0.65f;
    xTWEAKABLE F32 sPotBoilHeight = 0.3f;

    U32 replyToID;
} // namespace

static bool init;

#if DEBUG || RELEASE
void zCompLogicStirringPlayer::AddStaticTweaks()
{
    char* prefix = "Cooking Minigames|StirringPlayer|";
    xAUTOTWEAK(prefix, "sPotBoilStart", &sPotBoilStart, 0.0f, 10.0f, NULL, NULL, 0, true);
    xAUTOTWEAK(prefix, "sPotBoilHeight", &sPotBoilHeight, 0.0f, 10.0f, NULL, NULL, 0, true);
}
#endif

void zCompLogicStirringPlayer::Update(xScene*, F32 dt)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    xModelInstance* model = compNPCEntity->GetModel();
    xEntFrame* frame = compNPCEntity->GetFrame();

    static xResponseCurve potColorCurve;
    static const rgba_node potColors[3] = {
        { 0.0f, 64.0f, 255.0f, 64.0f, 255.0f },
        { 0.7f, 255.0f, 255.0f, 64.0f, 255.0f },
        { 1.0f, 255.0f, 64.0f, 64.0f, 255.0f }
    };

    if (!init) {
        if (compNPCEntity)
            compNPCEntity->SetAnimState("STIRRINGPLAYER_L_MOVE01", 0.3f, NULL);

        whichPot = targetPot = 1;
        for (S32 i = 0; i < 4; i++) {
            potTemp[i] = 112.0f;
            potPos[i] = sPotBoilStart;
            potRate[i] = 20.0f;
        }

        frame->mat.pos.x = potPositions[0].x;
        frame->mat.pos.y = potPositions[0].y;
        frame->mat.pos.z = potPositions[0].z;

        zEntEvent(xStrHash("CAM1"), eEventSwitch);

        potColorCurve.reset(4, potColors, 3);

        init = true;
    }

    xPad* pad = &mPad[0];
    if (whichPot == targetPot && xabs(pad->analog[1].mag) > 0.5f) {
        if (pad->analog[1].dir.x > 0.0f) {
            if (whichPot == 1)
                targetPot = 2;
            else if (whichPot == 4)
                targetPot = 3;
        } else if (pad->analog[1].dir.x < 0.0f) {
            if (whichPot == 2)
                targetPot = 1;
            else if (whichPot == 3)
                targetPot = 4;
        }

        if (pad->analog[1].dir.y > 0.0f) {
            if (whichPot == 1)
                targetPot = 4;
            else if (whichPot == 2)
                targetPot = 3;
        } else if (pad->analog[1].dir.y < 0.0f) {
            if (whichPot == 4)
                targetPot = 1;
            else if (whichPot == 3)
                targetPot = 2;
        }
    }

    if (whichPot == targetPot) {
        zMiniME& mgr = zMiniME::GetMgr();
        F32 frequency, duration;
        if (mgr.history.IsStirring(frequency, duration)) {
            potTemp[whichPot - 1] -= frequency * (10.0f * dt) / 5.0f;
            if (potTemp[whichPot - 1] < 112.0f)
                potTemp[whichPot - 1] = 112.0f;

            F32 speed = frequency;
            if (speed < 0.0f)
                speed = 0.0f;
            if (speed > 5.0f)
                speed = 5.0f;
            model->Anim->Single->CurrentSpeed = speed;
        } else
            model->Anim->Single->CurrentSpeed = 0.05f;
    } else {
        xVec3 pos, diff;
        pos.x = frame->mat.pos.x;
        pos.y = frame->mat.pos.y;
        pos.z = frame->mat.pos.z;
        xVec3Sub(&diff, &potPositions[targetPot - 1], &pos);
        xprintf("moving:  diff len == %6.3f\n", xVec3Length2(&diff));
        if (xVec3Length2(&diff) < 0.001f)
            whichPot = targetPot;
        xVec3AddScaled(&pos, &diff, 4.0f * dt);
        frame->mat.pos.x = pos.x;
        frame->mat.pos.y = pos.y;
        frame->mat.pos.z = pos.z;

        model->Anim->Single->CurrentSpeed = 0.0f;
    }

    for (S32 i = 0; i < 4; i++) {
        potTemp[i] += 0.04f * (potRate[i] * dt);
        potPos[i] = sPotBoilStart + sPotBoilHeight * (potTemp[i] - 112.0f) / 100.0f;

        if (potTemp[i] > 212.0f) {
            potTemp[i] = 212.0f;

            if (replyToID)
                zEntEvent(replyToID, eEventDispatcher_SLRetry);
        }

        static U32 boil[4] = { HASH_BOIL1, HASH_BOIL2, HASH_BOIL3, HASH_BOIL4 };
        xEnt* ent = (xEnt*)zSceneFindObject(boil[i]);
        if (ent) {
            rgba_node value;
            F32 frac = (potTemp[i] - 112.0f) / 100.0f;
            potColorCurve.eval_smooth(frac, &value.r);

            U8 r = value.r < 0.0f ? (U8)0 : (value.r < 255.0f ? (U8)value.r : (U8)255);
            U8 g = value.g < 0.0f ? (U8)0 : (value.g < 255.0f ? (U8)value.g : (U8)255);
            U8 b = value.b < 0.0f ? (U8)0 : (value.b < 255.0f ? (U8)value.b : (U8)255);

            RpGeometry* geom = ent->model->Data->geometry;
            RpMaterial* mater = geom->matList.materials[0];
            RpGeometrySetFlags(geom, RpGeometryGetFlags(geom) | rpGEOMETRYMODULATEMATERIALCOLOR);
            mater->color.red = r;
            mater->color.green = g;
            mater->color.blue = b;

            ent->model->Mat->pos.y = potPos[i];
        }
    }
}

bool zCompLogicStirringPlayer::SystemEvent(xBase*, xBase*, U32 to_event, const F32* to_param, xBase*, U32 to_param_widget_id)
{
    switch (to_event) {
    case eEventReset:
        potRate[0] = to_param[0];
        potRate[1] = to_param[1];
        potRate[2] = to_param[2];
        potRate[3] = to_param[3];
        break;
    case eEventAttachTo:
        replyToID = to_param_widget_id;
        break;
    }

    return false;
}
