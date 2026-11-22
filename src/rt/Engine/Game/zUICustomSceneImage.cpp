#include "xCommon.h"
#include "zUICustomSceneImage.h"

#include "containers.h"
#include "xGlobals.h"
#include "xPad.h"
#include "xString.h"
#include "zBase.h"
#include "zEvent.h"
#include "zHashes.h"
#include "zScene.h"
#include "zUI.h"
#include "zUICustomLevelManager.h"
#include "zUIImage.h"

#include <stdio.h>

char sceneBaseImage[32];

zUICustomSceneImage::zUICustomSceneImage(zUI* ui) : zUICustom(ui), counter(NULL)
{
    xASSERT_L(18, ui->asset->type == xStrHash("ui:image"));
}

void zUICustomSceneImage::Reset()
{
    counter = NULL;

    xBase* b;
    if (ui->asset->customWidget)
        b = zSceneFindObject(ui->asset->customWidget);
    else
        b = zSceneFindObject(HASH_UI_MISSION_COUNTER);

    if (b && b->baseType == eBaseTypeCounter) {
        counter = (xCounter*)b;
    }
}

bool zUICustomSceneImage::HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID)
{
    switch (toEvent) {
    case eEventUIChangeTexture:
        UpdateTexture();
        return false;
    }

    return zUICustom::HandleEvent(from, toEvent, toParam, toParamWidget, toParamWidgetID);
}

void zUICustomSceneImage::UpdateTexture()
{
    U32 sceneCurrent = zUIIncrediblesGetSceneCurrent();

    U32 scene = counter ? counter->count : sceneCurrent;
    if (scene > zUIIncrediblesGetNumScenes()) {
        scene = 0;
    }

    if (scene == 0) {
        char full[64], sceneStr[8];
        SCENEIDSTR(xglobals->sceneCur->sceneID, sceneStr);
        sprintf(full, "%s %s", sceneBaseImage, sceneStr);
        U32 imageID = xStrHash(full);
        ((zUIImage*)ui)->SetTexture(imageID, false);
    } else {
        Scene* sceneCur = zUIIncrediblesGetScene(scene);
        ((zUIImage*)ui)->SetTexture(sceneCur->imageID, false);
    }
}
