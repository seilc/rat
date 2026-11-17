#include "xCommon.h"
#include "zCompLogicPourNSwirl.h"

#include "xEvent.h"
#include "xFont.h"
#include "xPad.h"
#include "xString.h"
#include "zEvent.h"
#include "zICompNPCEntity.h"
#include "zScene.h"

namespace
{
    xEnt* small_glass;
    RwMatrix small_glass_mat;
    xEnt* large_glass;
    RwMatrix large_glass_mat;
} // namespace

static bool init;

#if DEBUG
void zCompLogicPourNSwirl::AddStaticTweaks()
{
}
#endif

void zCompLogicPourNSwirl::CreateAnimTable(xAnimTable*)
{
}

void zCompLogicPourNSwirl::Update(xScene*, F32)
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    xModelInstance* model = compNPCEntity->GetModel();
    xEntFrame* frame = compNPCEntity->GetFrame();

    if (!init) {
        small_glass = (xEnt*)zSceneFindObject(xStrHash("BEVERAGE_GLASS_SMALL_01"));
        if (small_glass)
            small_glass_mat = *small_glass->model->Mat;
        large_glass = (xEnt*)zSceneFindObject(xStrHash("BEVERAGE_GLASS_01"));
        if (large_glass)
            large_glass_mat = *large_glass->model->Mat;

        zEntEvent(xStrHash("POUR_CAM"), eEventSwitch);

        init = true;
    }

    xPad* pad = &mPad[0];
    if (large_glass) {
        xVec3 YPR = { 0.0f, 0.0f, 0.0f };
        if (pad->analog1.y > 0) {
            YPR.y = 90.0f * pad->analog1.y / 128.0f;
        }

        xVec3 pos = ((xMat4x3*)&large_glass_mat)->pos;
        xMat3x3Euler((xMat3x3*)&large_glass_mat, &YPR);
        xMat4x3SMul((xMat4x3*)&large_glass_mat, (xMat4x3*)&large_glass_mat, 0.1f);
        ((xMat4x3*)&large_glass_mat)->pos = pos;
    }
}
