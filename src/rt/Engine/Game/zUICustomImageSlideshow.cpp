#include "xCommon.h"
#include "zUICustomImageSlideshow.h"

#include "xGroup.h"
#include "xString.h"
#include "zBase.h"
#include "zEvent.h"
#include "zScene.h"
#include "zUI.h"
#include "zUIImage.h"

#include "decomp.h"

zUICustomImageSlideshow::zUICustomImageSlideshow(zUI* ui) : zUICustom(ui), m_CurrentImage(0), m_NumImages(0), m_TextureGroup(NULL)
{
    xASSERT_L(13, ui->asset->type == xStrHash("ui:image"));

    U32 widgetID = ui->GetAsset()->customWidget;
    if (widgetID)
        SetTextureList(widgetID);
}

void zUICustomImageSlideshow::Reset()
{
    m_CurrentImage = 0;
    if (m_TextureGroup)
        UpdateTexture();
}

bool zUICustomImageSlideshow::HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID)
{
    switch (toEvent) {
    case eEventUISignalUp:
        m_CurrentImage = m_CurrentImage < m_NumImages - 1 ? m_CurrentImage + 1 : 0;
        return true;

    case eEventUISignalDown:
        m_CurrentImage = m_CurrentImage ? m_CurrentImage - 1 : m_NumImages - 1;
        return true;

    case eEventUIChangeTexture:
        UpdateTexture();
        return false;

    default:
        return zUICustom::HandleEvent(from, toEvent, toParam, toParamWidget, toParamWidgetID);
    }
}

void zUICustomImageSlideshow::SetTextureList(U32 groupID)
{
    xASSERT_L(54, groupID);
    xBase* textureGroup = zSceneFindObject(groupID);
    xASSERT_L(56, textureGroup);
    xASSERT_L(57, textureGroup->baseType == eBaseTypeGroup);
    SetTextureList((xGroup*)textureGroup);
}

void zUICustomImageSlideshow::SetTextureList(xGroup* textureList)
{
    xASSERT_L(63, textureList);
    m_TextureGroup = textureList;
    m_CurrentImage = 0;
    m_NumImages = xGroupGetCount(m_TextureGroup);
    xASSERT_L(67, m_NumImages);
}

void zUICustomImageSlideshow::UpdateTexture()
{
    U32 textureID = xGroupGetItem(m_TextureGroup, m_CurrentImage);

    zUIImage* image = (zUIImage*)ui;
    image->SetTexture(textureID, false);
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("index < m_NumImages");
#endif
