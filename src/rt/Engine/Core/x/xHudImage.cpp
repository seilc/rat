#include "xHudImage.h"

#include "xHudAsset.h"
#include "xDebugTweak.h"
#include "xFont.h"
#include "xMath.h"
#include "xRand.h"
#include "xRenderBuffer.h"
#include "xRenderState.h"
#include "xScrFx.h"
#include "xstransvc.h"
#include "zMain.h"
#include "iDraw.h"
#include "iMath.h"

#include "decomp.h"

#define ADD_TRI(a, b, c)                        \
    *pIndex++ = m_uVertexCount + (a);           \
    *pIndex++ = m_uVertexCount + (b);           \
    *pIndex++ = m_uVertexCount + (c)

namespace xhud {

void image_widget::load(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate_L(41, asset, asset_size, image_asset, image_asset::type_name());
    init_base(data, asset, sizeof(widget_chunk));
    widget_chunk& wc = (widget_chunk&)data;
    new (&wc.w) image_widget((image_asset&)asset);
}

image_widget::image_widget(const image_asset& a)
    : widget(a)
    , m_pRaster(NULL)
    , m_pLayers(NULL)
    , m_paVertices(NULL)
    , m_paIndices(NULL)
    , m_uVertexCount(0)
    , m_uIndexCount(0)
    , m_fRecipZ(5.0f)
    , m_VertexMax(0)
    , m_IndexMax(0)
{
    RwTexture* pTexture = (RwTexture*)xSTFindAsset(a.image, NULL);
    xASSERT_FMT_L(60, pTexture != 0, "Could not load hud image: %s", xSTAssetNameOrID(a.image));

    m_pRaster = RwTextureGetRaster(pTexture);
}

void image_widget::destruct()
{
    m_pRaster = NULL;
    m_pLayers = NULL;
    m_paVertices = NULL;
    m_paIndices = NULL;
    m_VertexMax = 0;
    m_IndexMax = 0;
    widget::destruct();
}

void image_widget::destroy()
{
    destruct();
}

U32 image_widget::type() const
{
    static U32 myid = xStrHash(model_asset::type_name());
    return myid;
}

bool image_widget::is(U32 id) const
{
    return id == image_widget::type() || widget::is(id);
}

void image_widget::update(F32 dt)
{
    updater(dt);

    if (!visible() || rc.a <= 0.5f / 255.0f) {
        return;
    }

    for (U32 uLayer = 0; uLayer < m_uLayerCount; uLayer++) {
        if (m_pLayers[uLayer].eType == STROBE_EFFECT)
            update_strobe_effect(uLayer, dt);
    }
}

void image_widget::update_strobe_effect(U32 uLayer, F32 dt)
{
    xASSERT_L(108, m_pLayers[uLayer].fStrobeFreq);
    F32 fStrobeRangePixels = INSCREENY(rc.size.y);
    F32 fStrobeRangePercent = 1.0f + m_pLayers[uLayer].fStrobeHeight / fStrobeRangePixels;

    F32 fUpdateAmount = dt * fStrobeRangePercent / m_pLayers[uLayer].fStrobeFreq;
    m_pLayers[uLayer].fStrobe += fUpdateAmount;

    if (m_pLayers[uLayer].fStrobe > 1.0f) {
        if (m_pLayers[uLayer].nStrobeCount > 0) {
            m_pLayers[uLayer].nStrobeCount--;
        }

        m_pLayers[uLayer].fStrobe = 0.0f;
    }
}

void image_widget::render()
{
#if DEBUG || RELEASE
    bool bDebug = *(bool*)&rc.size.z;
    if (bDebug) {
        F32 rc_x0 = rc.loc.x * FB_XRES;
        F32 rc_y0 = rc.loc.y * FB_YRES;
        F32 rc_x2 = rc_x0 + rc.size.x * FB_XRES;
        F32 rc_y2 = rc_y0 + rc.size.y * FB_YRES;
        F32 rc_x1 = (rc_x0 + rc_x2) * 0.5f;
        F32 rc_y1 = (rc_y0 + rc_y2) * 0.5f;
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)NULL);
        xScrFxDrawBox(rc_x0, rc_y0, rc_x1, rc_y2, 192, 128, 128, 200, false);
        xScrFxDrawBox(rc_x1, rc_y0, rc_x2, rc_y2, 144, 144, 192, 200, false);
        return;
    }
#endif

    if (rc.a >= -0.00001f && rc.a <= 0.00001f) {
        return;
    }

    m_paVertices = gRenderBuffer.m_vertex_2d;
    xASSERT_L(147, m_paVertices);
    m_paIndices = (U16*)xMEMPUSHTEMP_L(m_IndexMax * sizeof(U16), -1, 148);
    xASSERT_L(149, m_paIndices);

    static RwReal fNearZ = RwIm2DGetNearScreenZ();
    m_fRecipZ = 5.0f;
    xASSERT_L(153, RwCameraGetCurrentCamera());
    if (RwCameraGetCurrentCamera()) {
        m_fRecipZ = 1.0f / RwCameraGetNearClipPlane(RwCameraGetCurrentCamera());
    }

    xRenderStateZModesSet(FALSE, FALSE);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLBACK);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, m_pRaster);

    m_uVertexCount = 0;
    m_uIndexCount = 0;
    m_pVertices = m_paVertices;

    LAYER_FLAGS eLayerMode = (LAYER_FLAGS)-1;
    xASSERT_L(174, m_uRenderLayerCount <=m_uLayerCount);
    for (U32 uLayer = 0; uLayer < m_uRenderLayerCount; uLayer++) {
        if (m_uVertexCount + 12 >= m_VertexMax) {
            flush();
        }
        if (m_uIndexCount + 24 >= m_IndexMax) {
            flush();
        }

        U32 uNewMode = m_pLayers[uLayer].uFlags & BLEND_MASK;
        if (eLayerMode != uNewMode) {
            gQuadBuffer->Flush();
            flush();

            eLayerMode = (LAYER_FLAGS)uNewMode;
            if (uNewMode == BLEND) {
                xRenderStateBlendModesSet(rwBLENDSRCALPHA, rwBLENDINVSRCALPHA);
                iDrawSetFBMSK(0xFF000000, 0xFFFFFFFF, true);
            }
            else if (uNewMode == ADDITIVE) {
                xRenderStateBlendModesSet(rwBLENDSRCALPHA, rwBLENDONE);
                iDrawSetFBMSK(0xFF000000, 0xFFFFFFFF, true);
            }
            else if (uNewMode == SET_DEST_ALPHA) {
                xRenderStateBlendModesSet(rwBLENDONE, rwBLENDZERO);
                iDrawSetFBMSK(0x00FFFFFF, 0xFFFFFFFF, true);
                RwGameCubeSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
            }
            else if (uNewMode == USE_DEST_ALPHA) {
                xRenderStateBlendModesSet(rwBLENDDESTALPHA, rwBLENDONE);
                iDrawSetFBMSK(0xFF000000, 0xFFFFFFFF, true);
            }
        }

        bool bRendered = false;
        switch (m_pLayers[uLayer].eType) {
        case TOPLEFT:
        case BOTTOMLEFT:
        case BOTTOMRIGHT:
        case TOPRIGHT:
        case OFFSET_CENTERPT:
            flush();
            bRendered = render_one_to_one(uLayer, m_pLayers[uLayer].eType);
            break;
        case OFFSET_PIE:
            gQuadBuffer->Flush();
            bRendered = render_offset_pie(uLayer);
            break;
        case ROTATE_CCW:
            flush();
            bRendered = render_rotate_ccw(uLayer);
            break;
        case OFFSET_ROTATE_CCW:
            flush();
            bRendered = render_offset_rotate_ccw(uLayer);
            break;
        case STROBE_EFFECT:
            flush();
            bRendered = render_strobe_effect(uLayer);
            break;
        case SPECIAL_STROBE:
            flush();
            bRendered = render_special_strobe(uLayer);
            break;
        case VARIABLE_TOP:
            flush();
            bRendered = render_variable_top(uLayer);
            break;
        case VARIABLE_RIGHT:
            flush();
            bRendered = render_variable_right(uLayer);
            break;
        case COUNTER_CLOCKWISE:
            gQuadBuffer->Flush();
            bRendered = render_counter_clockwise(uLayer);
            break;
        case HALF_CCW:
            gQuadBuffer->Flush();
            bRendered = render_half_ccw(uLayer);
            break;
        case RANDOM_V:
            flush();
            bRendered = render_random_v(uLayer);
            break;
        }

        if (bRendered) {
            set_buffer_to_layer(m_pLayers[uLayer].eType);
        }
    }
    flush();

    gQuadBuffer->Flush();

    xMEMPOPTEMP(m_paIndices);
    m_paIndices = NULL;

    m_paVertices = NULL;
    m_pVertices = NULL;
}

bool image_widget::render_one_to_one(U32 uLayer, RENDERTYPE eType)
{
    xASSERT_L(316, m_pLayers);
    xASSERT_L(317, uLayer < m_uLayerCount);
    if (!m_pLayers || uLayer >= m_uLayerCount)
        return false;
    if (0.0f == m_pLayers[uLayer].fAlpha && (m_pLayers[uLayer].uFlags & BLEND_MASK) != SET_DEST_ALPHA) {
        return false;
    }

    i2dRect uv_rect;
    uv_rect.tlu = m_pLayers[uLayer].upperLeft.u;
    uv_rect.tlv = m_pLayers[uLayer].upperLeft.v;
    uv_rect.bru = m_pLayers[uLayer].lowerRight.u;
    uv_rect.brv = m_pLayers[uLayer].lowerRight.v;

    i2dRect location_rect;
    location_rect.tlx = rc.loc.x * FB_XRES;
    location_rect.tly = rc.loc.y * FB_YRES;
    F32 fQuadWidth = (m_pLayers[uLayer].lowerRight.u - m_pLayers[uLayer].upperLeft.u) * get_texture_width();
    F32 fQuadHeight = (m_pLayers[uLayer].lowerRight.v - m_pLayers[uLayer].upperLeft.v) * get_texture_height();

    if (eType != TOPLEFT)
        if (eType == TOPRIGHT) {
            location_rect.tlx = rc.loc.x * FB_XRES + INSCREENX(rc.size.x) - fQuadWidth;
        }
        else if (eType == BOTTOMLEFT) {
            location_rect.tly = rc.loc.y * FB_YRES + INSCREENY(rc.size.y) - fQuadHeight;
        }
        else if (eType == BOTTOMRIGHT) {
            location_rect.tlx = rc.loc.x * FB_XRES + INSCREENX(rc.size.x) - fQuadWidth;
            location_rect.tly = rc.loc.y * FB_YRES + INSCREENY(rc.size.y) - fQuadHeight;
        }
        else if (eType == OFFSET_CENTERPT) {
            F32 fCenterX = rc.loc.x * FB_XRES + 0.5f * INSCREENX(rc.size.x);
            F32 fCenterY = rc.loc.y * FB_YRES + 0.5f * INSCREENY(rc.size.y);
            fQuadWidth *= m_pLayers[uLayer].fScaleX;
            fQuadHeight *= m_pLayers[uLayer].fScaleY;
            F32 fHalfImageWidth = 0.5f * fQuadWidth;
            F32 fHalfImageHeight = 0.5f * fQuadHeight;
            location_rect.tlx = fCenterX + m_pLayers[uLayer].fOffsetX - fHalfImageWidth;
            location_rect.tly = fCenterY + m_pLayers[uLayer].fOffsetY - fHalfImageHeight;
        }

    location_rect.brx = location_rect.tlx + fQuadWidth;
    location_rect.bry = location_rect.tly + fQuadHeight;

    xColor rect_color = g_WHITE;
    rect_color.a = rc.a * (255.0f * m_pLayers[uLayer].fAlpha);
    gQuadBuffer->Set(&location_rect, &uv_rect, rect_color);
    return true;
}

bool image_widget::render_variable_top(U32 uLayer)
{
    xASSERT_L(391, m_pLayers);
    xASSERT_L(392, uLayer < m_uLayerCount);
    if (!m_pLayers || uLayer >= m_uLayerCount)
        return false;
    if (0.0f == m_pLayers[uLayer].fAlpha && (m_pLayers[uLayer].uFlags & BLEND_MASK) != SET_DEST_ALPHA) {
        return false;
    }

    F32 fPercent = m_pLayers[uLayer].fCurPercent;
    i2dRect uv_rect;
    uv_rect.tlu = m_pLayers[uLayer].upperLeft.u;
    uv_rect.tlv = fPercent * (m_pLayers[uLayer].upperLeft.v - m_pLayers[uLayer].lowerRight.v) + m_pLayers[uLayer].lowerRight.v;
    uv_rect.bru = m_pLayers[uLayer].lowerRight.u;
    uv_rect.brv = m_pLayers[uLayer].lowerRight.v;

    i2dRect location_rect;
    F32 fQuadHeight = fPercent * INSCREENY(rc.size.y);
    location_rect.tly = rc.loc.y * FB_YRES + INSCREENY(rc.size.y) - fQuadHeight;
    F32 fQuadWidth = INSCREENX(rc.size.x);
    location_rect.tlx = rc.loc.x * FB_XRES;
    location_rect.brx = location_rect.tlx + fQuadWidth;
    location_rect.bry = location_rect.tly + fQuadHeight;

    xColor rect_color = g_WHITE;
    rect_color.a = rc.a * (255.0f * m_pLayers[uLayer].fAlpha);
    gQuadBuffer->Set(&location_rect, &uv_rect, rect_color);
    return true;
}

bool image_widget::render_variable_right(U32 uLayer)
{
    xASSERT_L(429, m_pLayers);
    xASSERT_L(430, uLayer < m_uLayerCount);
    if (!m_pLayers || uLayer >= m_uLayerCount)
        return false;
    if (0.0f == m_pLayers[uLayer].fAlpha && (m_pLayers[uLayer].uFlags & BLEND_MASK) != SET_DEST_ALPHA) {
        return false;
    }

    F32 fPercent = m_pLayers[uLayer].fCurPercent;

    i2dRect uv_rect;
    uv_rect.tlu = m_pLayers[uLayer].upperLeft.u;
    uv_rect.tlv = m_pLayers[uLayer].upperLeft.v;
    uv_rect.bru = fPercent * (m_pLayers[uLayer].lowerRight.u - m_pLayers[uLayer].upperLeft.u) + m_pLayers[uLayer].upperLeft.u;
    uv_rect.brv = m_pLayers[uLayer].lowerRight.v;

    i2dRect location_rect;
    location_rect.tlx = rc.loc.x * FB_XRES;
    location_rect.tly = rc.loc.y * FB_YRES;
    location_rect.brx = fPercent * INSCREENX(rc.size.x) + location_rect.tlx;
    location_rect.bry = location_rect.tly + INSCREENY(rc.size.y);

    xColor rect_color = g_WHITE;
    rect_color.a = rc.a * (255.0f * m_pLayers[uLayer].fAlpha);
    gQuadBuffer->Set(&location_rect, &uv_rect, rect_color);
    return true;
}

bool image_widget::render_counter_clockwise(U32 uLayer)
{
    xASSERT_L(463, m_pLayers);
    xASSERT_L(464, uLayer < m_uLayerCount);
    if (!m_pLayers || uLayer >= m_uLayerCount)
        return false;
    if (0.0f == m_pLayers[uLayer].fAlpha && (m_pLayers[uLayer].uFlags & BLEND_MASK) != SET_DEST_ALPHA) {
        return false;
    }

    F32 fPercent = m_pLayers[uLayer].fCurPercent;
    F32 u0 = m_pLayers[uLayer].upperLeft.u;
    F32 v0 = m_pLayers[uLayer].upperLeft.v;
    F32 u1 = m_pLayers[uLayer].lowerRight.u;
    F32 v1 = m_pLayers[uLayer].lowerRight.v;

    F32 fHalfWidth = INSCREENX(0.5f * rc.size.x);
    F32 fHalfHeight = INSCREENY(0.5f * rc.size.y);
    F32 fCenterX = rc.loc.x * FB_XRES + fHalfWidth;
    F32 fCenterY = rc.loc.y * FB_YRES + fHalfHeight;
    F32 fCenterYPlusfHalfHeight = fCenterY + fHalfHeight;
    F32 fCenterYMinusfHalfHeight = fCenterY - fHalfHeight;
    F32 fCenterU = 0.5f * (u0 + u1);
    F32 fCenterV = 0.5f * (v0 + v1);

    F32 fRangeToEight = 8.0f * fPercent;
    S32 nWholeTris = fRangeToEight;
    S32 nWholeTrisPlus1 = nWholeTris + 1;
    S32 nWholeTrisPlus2 = nWholeTris + 2;

    switch (nWholeTrisPlus1) {
    case 9:
    case 8:
        VertSetXYUV(m_pVertices[m_uVertexCount + 9], fCenterX, fCenterYMinusfHalfHeight, fCenterU, v0, m_fRecipZ);
    case 7:
        VertSetXYUV(m_pVertices[m_uVertexCount + 8], fCenterX + fHalfWidth, fCenterYMinusfHalfHeight, u1, v0, m_fRecipZ);
    case 6:
        VertSetXYUV(m_pVertices[m_uVertexCount + 7], fCenterX + fHalfWidth, fCenterY, u1, fCenterV, m_fRecipZ);
    case 5:
        VertSetXYUV(m_pVertices[m_uVertexCount + 6], fCenterX + fHalfWidth, fCenterYPlusfHalfHeight, u1, v1, m_fRecipZ);
    case 4:
        VertSetXYUV(m_pVertices[m_uVertexCount + 5], fCenterX, fCenterYPlusfHalfHeight, fCenterU, v1, m_fRecipZ);
    case 3:
        VertSetXYUV(m_pVertices[m_uVertexCount + 4], fCenterX - fHalfWidth, fCenterYPlusfHalfHeight, u0, v1, m_fRecipZ);
    case 2:
        VertSetXYUV(m_pVertices[m_uVertexCount + 3], fCenterX - fHalfWidth, fCenterY, u0, fCenterV, m_fRecipZ);
    case 1:
        VertSetXYUV(m_pVertices[m_uVertexCount + 2], fCenterX - fHalfWidth, fCenterYMinusfHalfHeight, u0, v0, m_fRecipZ);
        VertSetXYUV(m_pVertices[m_uVertexCount + 1], fCenterX, fCenterYMinusfHalfHeight, fCenterU, v0, m_fRecipZ);
        VertSetXYUV(m_pVertices[m_uVertexCount], fCenterX, fCenterY, fCenterU, fCenterV, m_fRecipZ);
        break;
    default:
        xFAIL_FMT_L(513, "%s", "Invalid case reached");
    }

    SetFourVertsAlpha(&m_pVertices[m_uVertexCount], rc.a);
    SetFourVertsAlpha(&m_pVertices[m_uVertexCount + 4], rc.a);
    RwIm2DVertexSetIntRGBA(&m_pVertices[m_uVertexCount + 8], 0xFF, 0xFF, 0xFF, 255.0f * rc.a);
    RwIm2DVertexSetIntRGBA(&m_pVertices[m_uVertexCount + 9], 0xFF, 0xFF, 0xFF, 255.0f * rc.a);

    if (nWholeTris < 8) {
        F32 fInterpAmount = fRangeToEight - nWholeTris;
        F32 xCoord, yCoord, uCoord, vCoord;
        S32 vertexIndex0 = m_uVertexCount + nWholeTris;
        S32 vertexIndex1 = m_uVertexCount + nWholeTrisPlus1;
        S32 vertexIndex2 = m_uVertexCount + nWholeTrisPlus2;

        if (nWholeTris & 1) {
            xCoord = 0.5f * (fInterpAmount * (m_pVertices[vertexIndex2].x - m_pVertices[vertexIndex0].x) + (m_pVertices[vertexIndex2].x + m_pVertices[vertexIndex0].x));
            yCoord = 0.5f * (fInterpAmount * (m_pVertices[vertexIndex2].y - m_pVertices[vertexIndex0].y) + (m_pVertices[vertexIndex2].y + m_pVertices[vertexIndex0].y));
            uCoord = 0.5f * (fInterpAmount * (m_pVertices[vertexIndex2].u - m_pVertices[vertexIndex0].u) + (m_pVertices[vertexIndex2].u + m_pVertices[vertexIndex0].u));
            vCoord = 0.5f * (fInterpAmount * (m_pVertices[vertexIndex2].v - m_pVertices[vertexIndex0].v) + (m_pVertices[vertexIndex2].v + m_pVertices[vertexIndex0].v));
        } else {
            xCoord = fInterpAmount * m_pVertices[vertexIndex2].x + (1.0f - fInterpAmount) * m_pVertices[vertexIndex1].x;
            yCoord = fInterpAmount * m_pVertices[vertexIndex2].y + (1.0f - fInterpAmount) * m_pVertices[vertexIndex1].y;
            uCoord = fInterpAmount * m_pVertices[vertexIndex2].u + (1.0f - fInterpAmount) * m_pVertices[vertexIndex1].u;
            vCoord = fInterpAmount * m_pVertices[vertexIndex2].v + (1.0f - fInterpAmount) * m_pVertices[vertexIndex1].v;
        }

        for (S32 VertIndex = 9; VertIndex >= nWholeTrisPlus2; VertIndex--)
            VertSetXYUV(m_pVertices[m_uVertexCount + VertIndex], xCoord, yCoord, uCoord, vCoord, m_fRecipZ);
    }

    return true;
}

bool image_widget::render_half_ccw(U32 uLayer)
{
    xASSERT_L(549, m_pLayers);
    xASSERT_L(550, uLayer < m_uLayerCount);
    if (!m_pLayers || uLayer >= m_uLayerCount)
        return false;
    if (0.0f == m_pLayers[uLayer].fAlpha && (m_pLayers[uLayer].uFlags & BLEND_MASK) != SET_DEST_ALPHA) {
        return false;
    }

    F32 fPercent = m_pLayers[uLayer].fCurPercent;

    F32 fClampTopY = rc.loc.y * FB_YRES + INSCREENY(rc.size.y) - m_pLayers[uLayer].fClampTopPercent * INSCREENY(rc.size.y);
    F32 fClampTopV = m_pLayers[uLayer].fClampTopPercent * (m_pLayers[uLayer].upperLeft.v - m_pLayers[uLayer].lowerRight.v) + m_pLayers[uLayer].lowerRight.v;

    F32 fHalfWidth = INSCREENX(0.5f * rc.size.x);
    F32 fHalfHeight = INSCREENY(0.5f * rc.size.y);
    F32 fCenterX = rc.loc.x * FB_XRES + fHalfWidth;
    F32 fCenterY = rc.loc.y * FB_YRES + fHalfHeight;
    F32 fCenterYPlusfHalfHeight = fCenterY + fHalfHeight;
    F32 fCenterYMinusfHalfHeight = fCenterY - fHalfHeight;

    fCenterY = fCenterY < fClampTopY ? fClampTopY : fCenterY;
    fCenterYPlusfHalfHeight = fCenterYPlusfHalfHeight < fClampTopY ? fClampTopY : fCenterYPlusfHalfHeight;
    fCenterYMinusfHalfHeight = fCenterYMinusfHalfHeight < fClampTopY ? fClampTopY : fCenterYMinusfHalfHeight;

    F32 u0 = m_pLayers[uLayer].upperLeft.u;
    F32 u1 = m_pLayers[uLayer].lowerRight.u;
    F32 fCenterU = 0.5f * (u0 + u1);

    F32 v0 = m_pLayers[uLayer].upperLeft.v;
    F32 v1 = m_pLayers[uLayer].lowerRight.v;
    F32 fCenterV = 0.5f * (v0 + v1);

    v0 = v0 < fClampTopV ? fClampTopV : v0;
    v1 = v1 < fClampTopV ? fClampTopV : v1;
    fCenterV = fCenterV < fClampTopV ? fClampTopV : fCenterV;

    F32 fRangeToFour = 4.0f * fPercent;
    S32 nWholeTris = fRangeToFour;
    S32 nWholeTrisPlus1 = nWholeTris + 1;
    S32 nWholeTrisPlus2 = nWholeTris + 2;

    VertSetXYUV(m_pVertices[m_uVertexCount + 11], fCenterX, fCenterYMinusfHalfHeight, fCenterU, v0, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 10], fCenterX + fHalfWidth, fCenterYMinusfHalfHeight, u1, v0, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 9], fCenterX + fHalfWidth, fCenterY, u1, fCenterV, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 8], fCenterX + fHalfWidth, fCenterYPlusfHalfHeight, u1, v1, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 7], fCenterX, fCenterYPlusfHalfHeight, fCenterU, v1, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 6], fCenterX, fCenterY, fCenterU, fCenterV, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 5], fCenterX, fCenterYPlusfHalfHeight, fCenterU, v1, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 4], fCenterX - fHalfWidth, fCenterYPlusfHalfHeight, u0, v1, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 3], fCenterX - fHalfWidth, fCenterY, u0, fCenterV, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 2], fCenterX - fHalfWidth, fCenterYMinusfHalfHeight, u0, v0, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 1], fCenterX, fCenterYMinusfHalfHeight, fCenterU, v0, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount], fCenterX, fCenterY, fCenterU, fCenterV, m_fRecipZ);
    SetFourVertsAlpha(&m_pVertices[m_uVertexCount], rc.a);
    SetFourVertsAlpha(&m_pVertices[m_uVertexCount + 4], rc.a);
    SetFourVertsAlpha(&m_pVertices[m_uVertexCount + 8], rc.a);

    if (nWholeTris < 4) {
        F32 fInterpAmount = fRangeToFour - nWholeTris;

        U32 vertexIndex0, vertexIndex1, vertexIndex2;
        F32 xCoord[2], yCoord[2], uCoord[2], vCoord[2];
        for (U32 uHalfOffset = 0; uHalfOffset <= 1; uHalfOffset++) {
            U32 uHalfOffsetX6 = uHalfOffset * 6;
            vertexIndex0 = m_uVertexCount + nWholeTris + uHalfOffsetX6;
            vertexIndex1 = m_uVertexCount + nWholeTrisPlus1 + uHalfOffsetX6;
            vertexIndex2 = m_uVertexCount + nWholeTrisPlus2 + uHalfOffsetX6;

            if (nWholeTris & 1) {
                xCoord[uHalfOffset] = 0.5f * (fInterpAmount * (m_pVertices[vertexIndex2].x - m_pVertices[vertexIndex0].x) + (m_pVertices[vertexIndex2].x + m_pVertices[vertexIndex0].x));
                yCoord[uHalfOffset] = 0.5f * (fInterpAmount * (m_pVertices[vertexIndex2].y - m_pVertices[vertexIndex0].y) + (m_pVertices[vertexIndex2].y + m_pVertices[vertexIndex0].y));
                uCoord[uHalfOffset] = 0.5f * (fInterpAmount * (m_pVertices[vertexIndex2].u - m_pVertices[vertexIndex0].u) + (m_pVertices[vertexIndex2].u + m_pVertices[vertexIndex0].u));
                vCoord[uHalfOffset] = 0.5f * (fInterpAmount * (m_pVertices[vertexIndex2].v - m_pVertices[vertexIndex0].v) + (m_pVertices[vertexIndex2].v + m_pVertices[vertexIndex0].v));
            } else {
                xCoord[uHalfOffset] = fInterpAmount * m_pVertices[vertexIndex2].x + (1.0f - fInterpAmount) * m_pVertices[vertexIndex1].x;
                yCoord[uHalfOffset] = fInterpAmount * m_pVertices[vertexIndex2].y + (1.0f - fInterpAmount) * m_pVertices[vertexIndex1].y;
                uCoord[uHalfOffset] = fInterpAmount * m_pVertices[vertexIndex2].u + (1.0f - fInterpAmount) * m_pVertices[vertexIndex1].u;
                vCoord[uHalfOffset] = fInterpAmount * m_pVertices[vertexIndex2].v + (1.0f - fInterpAmount) * m_pVertices[vertexIndex1].v;
            }

            for (U32 VertIndex = uHalfOffsetX6 + 5; VertIndex >= uHalfOffsetX6 + nWholeTrisPlus2; VertIndex--)
                VertSetXYUV(m_pVertices[m_uVertexCount + VertIndex], xCoord[uHalfOffset], yCoord[uHalfOffset], uCoord[uHalfOffset], vCoord[uHalfOffset], m_fRecipZ);
        }
    }

    return true;
}

bool image_widget::render_rotate_ccw(U32 uLayer)
{
    xASSERT_L(642, m_pLayers);
    xASSERT_L(643, uLayer < m_uLayerCount);
    if (!m_pLayers || uLayer >= m_uLayerCount)
        return false;
    if (0.0f == m_pLayers[uLayer].fAlpha && (m_pLayers[uLayer].uFlags & BLEND_MASK) != SET_DEST_ALPHA) {
        return false;
    }

    F32 fRadians = 2.0f * PI * -m_pLayers[uLayer].fCurPercent;

    F32 fTopU = m_pLayers[uLayer].upperLeft.u;
    F32 fTopV = m_pLayers[uLayer].upperLeft.v;
    F32 fBotU = m_pLayers[uLayer].lowerRight.u;
    F32 fBotV = m_pLayers[uLayer].lowerRight.v;

    F32 fTopX = rc.loc.x * FB_XRES;
    F32 fTopY = rc.loc.y * FB_YRES;

    F32 fCenterX = fTopX + m_pLayers[uLayer].fOffsetX;
    F32 fCenterY = fTopY + m_pLayers[uLayer].fOffsetY;

    F32 fBotX = fTopX + INSCREENX(rc.size.x);
    F32 fBotY = fTopY + INSCREENY(rc.size.y);

    xVec2 toTopLeft = { (fTopX - fCenterX) * m_pLayers[uLayer].fScaleX, (fTopY - fCenterY) * m_pLayers[uLayer].fScaleY };
    xVec2 toBotLeft = { (fTopX - fCenterX) * m_pLayers[uLayer].fScaleX, (fBotY - fCenterY) * m_pLayers[uLayer].fScaleY };
    xVec2 toTopRight = { (fBotX - fCenterX) * m_pLayers[uLayer].fScaleX, (fTopY - fCenterY) * m_pLayers[uLayer].fScaleY };
    xVec2 toBotRight = { (fBotX - fCenterX) * m_pLayers[uLayer].fScaleX, (fBotY - fCenterY) * m_pLayers[uLayer].fScaleY };
    F32 fSin = isin(fRadians);
    F32 fCos = icos(fRadians);

    F32 fTemp10 = toTopLeft.x * fCos - toTopLeft.y * fSin;
    F32 fTemp20 = toTopLeft.x * fSin + toTopLeft.y * fCos;
    toTopLeft.x = fTemp10;
    toTopLeft.y = fTemp20;

    fTemp10 = toBotLeft.x * fCos - toBotLeft.y * fSin;
    fTemp20 = toBotLeft.x * fSin + toBotLeft.y * fCos;
    toBotLeft.x = fTemp10;
    toBotLeft.y = fTemp20;

    fTemp10 = toTopRight.x * fCos - toTopRight.y * fSin;
    fTemp20 = toTopRight.x * fSin + toTopRight.y * fCos;
    toTopRight.x = fTemp10;
    toTopRight.y = fTemp20;

    fTemp10 = toBotRight.x * fCos - toBotRight.y * fSin;
    fTemp20 = toBotRight.x * fSin + toBotRight.y * fCos;
    toBotRight.x = fTemp10;
    toBotRight.y = fTemp20;

    VertSetXYUV(m_pVertices[m_uVertexCount], fCenterX + toTopLeft.x, fCenterY + toTopLeft.y, fTopU, fTopV, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 1], fCenterX + toTopRight.x, fCenterY + toTopRight.y, fBotU, fTopV, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 2], fCenterX + toBotLeft.x, fCenterY + toBotLeft.y, fTopU, fBotV, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 3], fCenterX + toBotRight.x, fCenterY + toBotRight.y, fBotU, fBotV, m_fRecipZ);
    SetFourVertsAlpha(&m_pVertices[m_uVertexCount], rc.a * m_pLayers[uLayer].fAlpha);
    return true;
}

bool image_widget::render_offset_rotate_ccw(U32 uLayer)
{
    xASSERT_L(702, m_pLayers);
    xASSERT_L(703, uLayer < m_uLayerCount);
    if (!m_pLayers || uLayer >= m_uLayerCount)
        return false;
    if (0.0f == m_pLayers[uLayer].fAlpha && (m_pLayers[uLayer].uFlags & BLEND_MASK) != SET_DEST_ALPHA) {
        return false;
    }

    F32 fRadians = 2.0f * PI * -m_pLayers[uLayer].fCurPercent;

    F32 fTopU = m_pLayers[uLayer].upperLeft.u;
    F32 fTopV = m_pLayers[uLayer].upperLeft.v;
    F32 fBotU = m_pLayers[uLayer].lowerRight.u;
    F32 fBotV = m_pLayers[uLayer].lowerRight.v;

    F32 fTopX = rc.loc.x * FB_XRES;
    F32 fTopY = rc.loc.y * FB_YRES;

    F32 fBotX = fTopX + INSCREENX(rc.size.x);
    F32 fBotY = fTopY + INSCREENY(rc.size.y);

    F32 fCenterX = 0.5f * (fTopX + fBotX);
    F32 fCenterY = 0.5f * (fTopY + fBotY);

    xVec2 toTopLeft = { (fTopX - fCenterX) * m_pLayers[uLayer].fScaleX, (fTopY - fCenterY) * m_pLayers[uLayer].fScaleY };
    xVec2 toBotLeft = { (fTopX - fCenterX) * m_pLayers[uLayer].fScaleX, (fBotY - fCenterY) * m_pLayers[uLayer].fScaleY };
    xVec2 toTopRight = { (fBotX - fCenterX) * m_pLayers[uLayer].fScaleX, (fTopY - fCenterY) * m_pLayers[uLayer].fScaleY };
    xVec2 toBotRight = { (fBotX - fCenterX) * m_pLayers[uLayer].fScaleX, (fBotY - fCenterY) * m_pLayers[uLayer].fScaleY };
    F32 fSin = isin(fRadians);
    F32 fCos = icos(fRadians);

    F32 fTemp10 = toTopLeft.x * fCos - toTopLeft.y * fSin;
    F32 fTemp20 = toTopLeft.x * fSin + toTopLeft.y * fCos;
    toTopLeft.x = fTemp10;
    toTopLeft.y = fTemp20;

    fTemp10 = toBotLeft.x * fCos - toBotLeft.y * fSin;
    fTemp20 = toBotLeft.x * fSin + toBotLeft.y * fCos;
    toBotLeft.x = fTemp10;
    toBotLeft.y = fTemp20;

    fTemp10 = toTopRight.x * fCos - toTopRight.y * fSin;
    fTemp20 = toTopRight.x * fSin + toTopRight.y * fCos;
    toTopRight.x = fTemp10;
    toTopRight.y = fTemp20;

    fTemp10 = toBotRight.x * fCos - toBotRight.y * fSin;
    fTemp20 = toBotRight.x * fSin + toBotRight.y * fCos;
    toBotRight.x = fTemp10;
    toBotRight.y = fTemp20;

    VertSetXYUV(m_pVertices[m_uVertexCount], fCenterX + toTopLeft.x + m_pLayers[uLayer].fOffsetX, fCenterY + toTopLeft.y + m_pLayers[uLayer].fOffsetY, fTopU, fTopV, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 1], fCenterX + toTopRight.x + m_pLayers[uLayer].fOffsetX, fCenterY + toTopRight.y + m_pLayers[uLayer].fOffsetY, fBotU, fTopV, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 2], fCenterX + toBotLeft.x + m_pLayers[uLayer].fOffsetX, fCenterY + toBotLeft.y + m_pLayers[uLayer].fOffsetY, fTopU, fBotV, m_fRecipZ);
    VertSetXYUV(m_pVertices[m_uVertexCount + 3], fCenterX + toBotRight.x + m_pLayers[uLayer].fOffsetX, fCenterY + toBotRight.y + m_pLayers[uLayer].fOffsetY, fBotU, fBotV, m_fRecipZ);
    SetFourVertsAlpha(&m_pVertices[m_uVertexCount], rc.a * m_pLayers[uLayer].fAlpha);
    return true;
}

bool image_widget::render_offset_pie(U32 uLayer)
{
    xASSERT_L(762, m_pLayers);
    xASSERT_L(763, uLayer < m_uLayerCount);
    if (!m_pLayers || uLayer >= m_uLayerCount)
        return false;
    if (0.0f == m_pLayers[uLayer].fAlpha && (m_pLayers[uLayer].uFlags & BLEND_MASK) != SET_DEST_ALPHA) {
        return false;
    }

    F32 u0 = m_pLayers[uLayer].upperLeft.u;
    F32 v0 = m_pLayers[uLayer].upperLeft.v;
    F32 u1 = m_pLayers[uLayer].lowerRight.u;
    F32 v1 = m_pLayers[uLayer].lowerRight.v;

    F32 fQuadWidth = (m_pLayers[uLayer].lowerRight.u - m_pLayers[uLayer].upperLeft.u) * get_texture_width();
    F32 fQuadHeight = (m_pLayers[uLayer].lowerRight.v - m_pLayers[uLayer].upperLeft.v) * get_texture_height();

    F32 fCenterX = m_pLayers[uLayer].fOffsetX + (rc.loc.x * FB_XRES + 0.5f * INSCREENX(rc.size.x));
    F32 fCenterY = m_pLayers[uLayer].fOffsetY + (rc.loc.y * FB_YRES + 0.5f * INSCREENY(rc.size.y));

    fQuadWidth *= m_pLayers[uLayer].fScaleX;
    fQuadHeight *= m_pLayers[uLayer].fScaleY;

    F32 fHalfWidth = 0.5f * fQuadWidth;
    F32 fHalfHeight = 0.5f * fQuadHeight;

    F32 fQuadX = fCenterX + m_pLayers[uLayer].fOffsetX - fHalfWidth;
    F32 fQuadY = fCenterY + m_pLayers[uLayer].fOffsetY - fHalfHeight;

    F32 fPercent = m_pLayers[uLayer].fCurPercent;

    F32 fCenterYPlusfHalfHeight = fCenterY + fHalfHeight;
    F32 fCenterYMinusfHalfHeight = fCenterY - fHalfHeight;
    F32 fCenterU = 0.5f * (u0 + u1);
    F32 fCenterV = 0.5f * (v0 + v1);

    F32 fRangeToEight = 8.0f * fPercent;
    S32 nWholeTris = fRangeToEight;
    S32 nWholeTrisPlus1 = nWholeTris + 1;
    S32 nWholeTrisPlus2 = nWholeTris + 2;

    switch (nWholeTrisPlus1) {
    case 9:
    case 8:
        VertSetXYUV(m_pVertices[m_uVertexCount + 9], fCenterX, fCenterYMinusfHalfHeight, fCenterU, v0, m_fRecipZ);
    case 7:
        VertSetXYUV(m_pVertices[m_uVertexCount + 8], fCenterX + fHalfWidth, fCenterYMinusfHalfHeight, u1, v0, m_fRecipZ);
    case 6:
        VertSetXYUV(m_pVertices[m_uVertexCount + 7], fCenterX + fHalfWidth, fCenterY, u1, fCenterV, m_fRecipZ);
    case 5:
        VertSetXYUV(m_pVertices[m_uVertexCount + 6], fCenterX + fHalfWidth, fCenterYPlusfHalfHeight, u1, v1, m_fRecipZ);
    case 4:
        VertSetXYUV(m_pVertices[m_uVertexCount + 5], fCenterX, fCenterYPlusfHalfHeight, fCenterU, v1, m_fRecipZ);
    case 3:
        VertSetXYUV(m_pVertices[m_uVertexCount + 4], fCenterX - fHalfWidth, fCenterYPlusfHalfHeight, u0, v1, m_fRecipZ);
    case 2:
        VertSetXYUV(m_pVertices[m_uVertexCount + 3], fCenterX - fHalfWidth, fCenterY, u0, fCenterV, m_fRecipZ);
    case 1:
        VertSetXYUV(m_pVertices[m_uVertexCount + 2], fCenterX - fHalfWidth, fCenterYMinusfHalfHeight, u0, v0, m_fRecipZ);
        VertSetXYUV(m_pVertices[m_uVertexCount + 1], fCenterX, fCenterYMinusfHalfHeight, fCenterU, v0, m_fRecipZ);
        VertSetXYUV(m_pVertices[m_uVertexCount], fCenterX, fCenterY, fCenterU, fCenterV, m_fRecipZ);
        break;
    default:
        xFAIL_FMT_L(821, "%s", "Invalid case reached");
    }

    SetFourVertsAlpha(&m_pVertices[m_uVertexCount], rc.a);
    SetFourVertsAlpha(&m_pVertices[m_uVertexCount + 4], rc.a);
    RwIm2DVertexSetIntRGBA(&m_pVertices[m_uVertexCount + 8], 0xFF, 0xFF, 0xFF, 255.0f * rc.a);
    RwIm2DVertexSetIntRGBA(&m_pVertices[m_uVertexCount + 9], 0xFF, 0xFF, 0xFF, 255.0f * rc.a);

    if (nWholeTris < 8) {
        F32 fInterpAmount = fRangeToEight - nWholeTris;
        F32 xCoord, yCoord, uCoord, vCoord;
        S32 vertexIndex0 = m_uVertexCount + nWholeTris;
        S32 vertexIndex1 = m_uVertexCount + nWholeTrisPlus1;
        S32 vertexIndex2 = m_uVertexCount + nWholeTrisPlus2;

        if (nWholeTris & 1) {
            xCoord = 0.5f * (fInterpAmount * (m_pVertices[vertexIndex2].x - m_pVertices[vertexIndex0].x) + (m_pVertices[vertexIndex2].x + m_pVertices[vertexIndex0].x));
            yCoord = 0.5f * (fInterpAmount * (m_pVertices[vertexIndex2].y - m_pVertices[vertexIndex0].y) + (m_pVertices[vertexIndex2].y + m_pVertices[vertexIndex0].y));
            uCoord = 0.5f * (fInterpAmount * (m_pVertices[vertexIndex2].u - m_pVertices[vertexIndex0].u) + (m_pVertices[vertexIndex2].u + m_pVertices[vertexIndex0].u));
            vCoord = 0.5f * (fInterpAmount * (m_pVertices[vertexIndex2].v - m_pVertices[vertexIndex0].v) + (m_pVertices[vertexIndex2].v + m_pVertices[vertexIndex0].v));
        } else {
            xCoord = fInterpAmount * m_pVertices[vertexIndex2].x + (1.0f - fInterpAmount) * m_pVertices[vertexIndex1].x;
            yCoord = fInterpAmount * m_pVertices[vertexIndex2].y + (1.0f - fInterpAmount) * m_pVertices[vertexIndex1].y;
            uCoord = fInterpAmount * m_pVertices[vertexIndex2].u + (1.0f - fInterpAmount) * m_pVertices[vertexIndex1].u;
            vCoord = fInterpAmount * m_pVertices[vertexIndex2].v + (1.0f - fInterpAmount) * m_pVertices[vertexIndex1].v;
        }

        for (S32 VertIndex = 9; VertIndex >= nWholeTrisPlus2; VertIndex--)
            VertSetXYUV(m_pVertices[m_uVertexCount + VertIndex], xCoord, yCoord, uCoord, vCoord, m_fRecipZ);
    }

    return true;
}

bool image_widget::render_strobe_effect(U32 uLayer)
{
    xASSERT_L(858, m_pLayers);
    xASSERT_L(859, uLayer < m_uLayerCount);
    if (!m_pLayers || uLayer >= m_uLayerCount)
        return false;
    if (0.0f == m_pLayers[uLayer].fAlpha && (m_pLayers[uLayer].uFlags & BLEND_MASK) != SET_DEST_ALPHA)
        return false;
    if (m_pLayers[uLayer].nStrobeCount == 0) {
        return false;
    }

    i2dRect location_rect;

    F32 fRealBottomY = INSCREENY(rc.size.y);
    F32 fVirtualBottomY = fRealBottomY + m_pLayers[uLayer].fStrobeHeight;
    F32 fStrobeRangePixels = (m_pLayers[uLayer].fMaxPercent - m_pLayers[uLayer].fMinPercent) * INSCREENY(rc.size.y) + m_pLayers[uLayer].fStrobeHeight;
    location_rect.bry = fVirtualBottomY - fStrobeRangePixels * m_pLayers[uLayer].fStrobe;
    location_rect.tly = location_rect.bry - m_pLayers[uLayer].fStrobeHeight;
    if (location_rect.bry > fRealBottomY) {
        location_rect.bry = fRealBottomY;
    }

    F32 fRealTopY = (1.0f - m_pLayers[uLayer].fCurPercent) * INSCREENY(rc.size.y);
    if (fRealTopY >= location_rect.bry) {
        location_rect.tly = location_rect.bry;
    }
    else if (location_rect.tly < fRealTopY) {
        location_rect.tly = fRealTopY;
    }

    F32 fTopYPercent = location_rect.tly / INSCREENY(rc.size.y);
    F32 fBottomYPercent = location_rect.bry / INSCREENY(rc.size.y);

    F32 vRange = m_pLayers[uLayer].lowerRight.v - m_pLayers[uLayer].upperLeft.v;
    i2dRect uv_rect;
    uv_rect.tlu = m_pLayers[uLayer].upperLeft.u;
    uv_rect.tlv = fTopYPercent * vRange + m_pLayers[uLayer].upperLeft.v;
    uv_rect.bru = m_pLayers[uLayer].lowerRight.u;
    uv_rect.brv = fBottomYPercent * vRange + m_pLayers[uLayer].upperLeft.v;

    location_rect.tly = rc.loc.y * FB_YRES + location_rect.tly;
    location_rect.bry = rc.loc.y * FB_YRES + location_rect.bry;

    location_rect.tlx = rc.loc.x * FB_XRES;
    location_rect.brx = location_rect.tlx + INSCREENX(rc.size.x);

    xColor rect_color = g_WHITE;
    rect_color.a = rc.a * (255.0f * m_pLayers[uLayer].fAlpha);
    gQuadBuffer->Set(&location_rect, &uv_rect, rect_color);
    return true;
}

bool image_widget::render_special_strobe(U32 uLayer)
{
    xASSERT_L(917, m_pLayers);
    xASSERT_L(918, uLayer < m_uLayerCount);
    if (!m_pLayers || uLayer >= m_uLayerCount)
        return false;
    if (0.0f == m_pLayers[uLayer].fAlpha && (m_pLayers[uLayer].uFlags & BLEND_MASK) != SET_DEST_ALPHA) {
        return false;
    }

    i2dRect location_rect;

    F32 fRealBottomY = INSCREENY(rc.size.y);
    F32 fVirtualBottomY = fRealBottomY + m_pLayers[uLayer].fStrobeHeight;
    F32 fStrobeRangePixels = (m_pLayers[uLayer].fMaxPercent - m_pLayers[uLayer].fMinPercent) * INSCREENY(rc.size.y) + m_pLayers[uLayer].fStrobeHeight;
    location_rect.bry = fVirtualBottomY - fStrobeRangePixels * m_pLayers[uLayer].fStrobe;
    location_rect.tly = location_rect.bry - m_pLayers[uLayer].fStrobeHeight;
    if (location_rect.bry > fRealBottomY) {
        location_rect.bry = fRealBottomY;
    }

    F32 fRealTopY = 0.0f;
    if (location_rect.tly < fRealTopY) {
        location_rect.tly = fRealTopY;
    }

    F32 fTopYPercent = location_rect.tly / INSCREENY(rc.size.y);
    F32 fBottomYPercent = location_rect.bry / INSCREENY(rc.size.y);

    i2dRect uv_rect;
    uv_rect.tlu = m_pLayers[uLayer].upperLeft.u;
    uv_rect.tlv = m_pLayers[uLayer].upperLeft.v;
    uv_rect.bru = m_pLayers[uLayer].lowerRight.u;
    uv_rect.brv = m_pLayers[uLayer].lowerRight.v;

    location_rect.tly = rc.loc.y * FB_YRES + location_rect.tly;
    location_rect.bry = rc.loc.y * FB_YRES + location_rect.bry;

    location_rect.tlx = rc.loc.x * FB_XRES;
    location_rect.brx = location_rect.tlx + INSCREENX(rc.size.x);

    xColor rect_color = g_WHITE;
    rect_color.a = rc.a * (255.0f * m_pLayers[uLayer].fAlpha);
    gQuadBuffer->Set(&location_rect, &uv_rect, rect_color);
    return true;
}

bool image_widget::render_random_v(U32 uLayer)
{
    xASSERT_L(967, m_pLayers);
    xASSERT_L(968, uLayer < m_uLayerCount);
    if (!m_pLayers || uLayer >= m_uLayerCount)
        return false;
    if (0.0f == m_pLayers[uLayer].fAlpha && (m_pLayers[uLayer].uFlags & BLEND_MASK) != SET_DEST_ALPHA) {
        return false;
    }

    i2dRect location_rect;
    location_rect.tlx = rc.loc.x * FB_XRES;
    location_rect.tly = rc.loc.y * FB_YRES;
    location_rect.bry = location_rect.tly + INSCREENY(rc.size.y);

    F32 fWidth = INSCREENX(rc.size.x) - 1.0f;
    location_rect.brx = location_rect.tlx + fWidth;

    F32 fDeltaV = fWidth / get_texture_height();

    i2dRect uv_rect;
    uv_rect.tlu = m_pLayers[uLayer].upperLeft.u;
    uv_rect.tlv = xrand_RandomFloatRange(m_pLayers[uLayer].upperLeft.v, m_pLayers[uLayer].lowerRight.v);
    uv_rect.bru = m_pLayers[uLayer].lowerRight.u;
    uv_rect.brv = uv_rect.tlv + fDeltaV;

    xColor rect_color = g_WHITE;
    rect_color.a = rc.a * (255.0f * m_pLayers[uLayer].fAlpha);
    gQuadBuffer->Set(&location_rect, &uv_rect, rect_color);
    return true;
}

// UNUSED: ported from Incredibles GC
bool image_widget::set_layer_uvs(U32 whichLayer, RwTexCoords& upperLeft, RwTexCoords& lowerRight)
{
    xASSERT(m_pLayers);
    xASSERT(whichLayer < m_uLayerCount);
    if (!m_pLayers || whichLayer >= m_uLayerCount) {
        return false;
    }

    m_pLayers[whichLayer].upperLeft = upperLeft;
    m_pLayers[whichLayer].lowerRight = lowerRight;
    return true;
}

void image_widget::set_buffer_to_layer(RENDERTYPE eType)
{
    U16* pIndex = &m_paIndices[m_uIndexCount];

    switch (eType) {
    case TOPLEFT:
    case BOTTOMLEFT:
    case BOTTOMRIGHT:
    case TOPRIGHT:
    case OFFSET_CENTERPT:
    case STROBE_EFFECT:
    case ROTATE_CCW:
    case OFFSET_ROTATE_CCW:
    case VARIABLE_TOP:
    case VARIABLE_RIGHT:
    case RANDOM_V:
    case SPECIAL_STROBE:
        return;
    case OFFSET_PIE:
    case COUNTER_CLOCKWISE:
        ADD_TRI(0, 1, 2);
        ADD_TRI(0, 2, 3);
        ADD_TRI(0, 3, 4);
        ADD_TRI(0, 4, 5);
        ADD_TRI(0, 5, 6);
        ADD_TRI(0, 6, 7);
        ADD_TRI(0, 7, 8);
        ADD_TRI(0, 8, 9);

        m_uVertexCount += 10;
        m_uIndexCount += 24;
        break;
    case HALF_CCW:
        ADD_TRI(0, 1, 2);
        ADD_TRI(0, 2, 3);
        ADD_TRI(0, 3, 4);
        ADD_TRI(0, 4, 5);
        ADD_TRI(6, 7, 8);
        ADD_TRI(6, 8, 9);
        ADD_TRI(6, 9, 10);
        ADD_TRI(6, 10, 11);

        m_uVertexCount += 12;
        m_uIndexCount += 24;
        break;
    }
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("Hud|%s|Layer %02d|Override Value", "Hud|%s|Layer %02d|Override Alpha", "Hud|%s|Layer %02d|Override XOffset",
                   "Hud|%s|Layer %02d|Override YOffset", "Hud|%s|Layer %02d|Override XScale", "Hud|%s|Layer %02d|Override YScale")
#endif

}
