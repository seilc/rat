#include "xScrPulseGlare.h"

#include "xRand.h"
#include "xIM.h"

#include <cmath>

#define zMAX_PULSE_GLARES 128

namespace {

    struct xPulse
    {
        F32 max_amp;
        F32 elapsed;
        F32 period;
        F32 freq;
        F32 noise;
    };

    struct xPulseGlare
    {
        struct config
        {
            F32 vis_angle;
            F32 amp_freq;
            F32 rot_freg;
        };

        config cfg;
        void(*cb_update_size)(xPulse&, F32);
        void(*cb_update_rot)(xPulse&, F32);
        xVec3 pos;
        xVec3 dir;
        F32 scale_dir;
        F32 elapsed;
        F32 lifetime;
        F32 size;
        F32 angle;
        xPulse p;
        S32 flags;
        RwRaster* raster;
        RwRGBAReal col;
    };

}

struct xPulseGlareMgr
{
public:
    void init();

#ifndef NON_MATCHING
    xPulseGlare* get(U32 i);
#endif

    void update(F32 dt);
    void render(const xMat4x3& mat);

protected:
    xPulseGlare glares[zMAX_PULSE_GLARES];
};

namespace {

    xPulseGlareMgr glare_mgr;

    void init_pulse(xPulse& p)
    {
        p.max_amp = 200.0f;
        p.elapsed = 0.0f;
        p.period = 2.0f;
        p.freq = 20.0f;
        p.noise = 0.0f;
    }

    void xPulseFn(xPulse& p, F32)
    {
        p.noise = (S32)xrand_RandomInt32() % 100 / 4000.0f;
    }

    void init_pulse_glare(xPulseGlare& glare)
    {
        glare.cfg.amp_freq = 30.0f;
        glare.cfg.rot_freg = 1.0f;
        glare.cfg.vis_angle = std::cosf(90.0f);

        glare.lifetime = 0.1f;
        glare.elapsed = 0.0f;
        glare.raster = NULL;
        glare.cb_update_size = xPulseFn;
        glare.cb_update_rot = NULL;
        glare.flags = 0;
        glare.size = 0.3f;
        glare.col.red = 1.0f;
        glare.col.green = 1.0f;
        glare.col.blue = 1.0f;
        glare.col.alpha = 1.0f;
        glare.scale_dir = 1.0f;
        glare.angle = 0.0f;

        init_pulse(glare.p);
    }

    void update_pulse_glare(xPulseGlare& g, F32 dt)
    {
        if (g.flags & 0x2) {
            g.elapsed += dt;
            if (g.lifetime < g.elapsed) {
                g.elapsed = 0.0f;
                g.flags &= ~0x2;
                g.flags &= ~0x1;
            }
        }

        if (g.cb_update_size) {
            g.cb_update_size(g.p, dt);
        }

        g.angle += 0.2f;
        g.angle = (F32)((S32)g.angle % 360);
    }

    // Equivalent: scheduling
    void render_pulse_glare(const xMat4x3& mat, xPulseGlare* g)
    {
        xVec3 w, h;
        h = mat.right;
        w = mat.up;

        xVec3 g_pos = g->pos;

        if (g->flags & 0x20) {
            g_pos.AddScale(g->dir, g->scale_dir);
        }

        xVec3Normalize(&h, &h);
        xVec3Normalize(&w, &w);

        xVec3 cam_dir;
        cam_dir.x = mat.pos.x - g_pos.x;
        cam_dir.y = mat.pos.y - g_pos.y;
        cam_dir.z = mat.pos.z - g_pos.z;

        xVec3Normalize(&g->dir, &g->dir);
        xVec3Normalize(&cam_dir, &cam_dir);

        if (!(g->flags & 0x40)) {
            xVec3Cross(&h, &w, &g->dir);
            xVec3Normalize(&h, &h);
        }

        F32 dot = g->dir.dot(cam_dir);
        xASSERTMSG(137, dot <= 1.0f, " vectors are not normalized !!!");

        if (g->flags & 0x10) {
            dot = 1.0f - dot;
        }

        F32 noise = 0.0f;
        if (g->flags & 0x80) {
            noise = g->p.noise;
        }

        if (!(g->flags & 0x4)) {
            if (dot <= g->cfg.vis_angle) return;

            h *= (g->size + noise) * dot;
            w *= (g->size + noise) * dot;
        } else {
            h *= g->size + noise;
            w *= g->size + noise;
        }

        if (g->flags & 0x8) {
            xMat3x3 m;
            xMat3x3Rot(&m, &mat.at, 45.0f * dot * 0.5f * (1/(22/7.f)));
            xMat3x3RMulVec(&h, &m, &h);
            xMat3x3RMulVec(&w, &m, &w);
        }

        static RwIm3DVertex sStripVert[4];
        RwIm3DVertex* vert = sStripVert;
        {
            RwV3d tmp;
            tmp.x = g_pos.x - w.x - h.x;
            tmp.y = g_pos.y - w.y - h.y;
            tmp.z = g_pos.z - w.z - h.z;
            RwIm3DVertexSetPos(vert, tmp.x, tmp.y, tmp.z);
            RwIm3DVertexSetUV(vert, 0.0f, 0.0f);

            RwRGBA col;
            col.red = 255.0f * g->col.red;
            col.green = 255.0f * g->col.green;
            col.blue = 255.0f * g->col.blue;
            col.alpha = 255.0f * g->col.alpha;
            RwIm3DVertexSetRGBA(vert, col.red, col.green, col.blue, col.alpha);
        }
        vert++;
        {
            RwV3d tmp;
            tmp.x = g_pos.x - w.x + h.x;
            tmp.y = g_pos.y - w.y + h.y;
            tmp.z = g_pos.z - w.z + h.z;
            RwIm3DVertexSetPos(vert, tmp.x, tmp.y, tmp.z);
            RwIm3DVertexSetUV(vert, 0.0f, 1.0f);

            RwRGBA col;
            col.red = 255.0f * g->col.red;
            col.green = 255.0f * g->col.green;
            col.blue = 255.0f * g->col.blue;
            col.alpha = 255.0f * g->col.alpha;
            RwIm3DVertexSetRGBA(vert, col.red, col.green, col.blue, col.alpha);
        }
        vert++;
        {
            RwV3d tmp;
            tmp.x = g_pos.x + w.x - h.x;
            tmp.y = g_pos.y + w.y - h.y;
            tmp.z = g_pos.z + w.z - h.z;
            RwIm3DVertexSetPos(vert, tmp.x, tmp.y, tmp.z);
            RwIm3DVertexSetUV(vert, 1.0f, 0.0f);

            RwRGBA col;
            col.red = 255.0f * g->col.red;
            col.green = 255.0f * g->col.green;
            col.blue = 255.0f * g->col.blue;
            col.alpha = 255.0f * g->col.alpha;
            RwIm3DVertexSetRGBA(vert, col.red, col.green, col.blue, col.alpha);
        }
        vert++;
        {
            RwV3d tmp;
            tmp.x = g_pos.x + w.x + h.x;
            tmp.y = g_pos.y + w.y + h.y;
            tmp.z = g_pos.z + w.z + h.z;
            RwIm3DVertexSetPos(vert, tmp.x, tmp.y, tmp.z);
            RwIm3DVertexSetUV(vert, 1.0f, 1.0f);

            RwRGBA col;
            col.red = 255.0f * g->col.red;
            col.green = 255.0f * g->col.green;
            col.blue = 255.0f * g->col.blue;
            col.alpha = 255.0f * g->col.alpha;
            RwIm3DVertexSetRGBA(vert, col.red, col.green, col.blue, col.alpha);
        }
        vert++;
        
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)g->raster);
        xIMRenderLegacy(sStripVert, 4, NULL, rwPRIMTYPETRISTRIP);
    }

}

void xPulseGlareMgr::init()
{
    for (S32 i = 0; i < zMAX_PULSE_GLARES; i++) {
        init_pulse_glare(glares[i]);
    }
}

xPulseGlare* xPulseGlareMgr::get(U32 i)
{
    xASSERT(0, !(i < 0 || i >= zMAX_PULSE_GLARES ));
}

void xPulseGlareMgr::update(F32 dt)
{
    for (S32 i = 0; i < zMAX_PULSE_GLARES; i++) {
        if (glares[i].flags & 0x1) {
            update_pulse_glare(glares[i], dt);
        }
    }
}

void xPulseGlareMgr::render(const xMat4x3& mat)
{
    for (S32 i = 0; i < zMAX_PULSE_GLARES; i++) {
        if (glares[i].flags & 0x1) {
            render_pulse_glare(mat, &glares[i]);
        }
    }
}

#ifndef NON_MATCHING
void xScrFX_PulseGlaresAdd(RwRaster*)
{
    xASSERTMSG(0, 0, " GLARE Manager run out of available slots !");

    S32 idx = -1;
    xASSERT(0, idx > -1);
}
#endif

void xScrFX_PulseGlaresRender(const xMat4x3& mat)
{
    glare_mgr.render(mat);
}

void xScrFX_PulseGlaresUpdate(F32 dt)
{
    glare_mgr.update(dt);
}

void xScrFX_PulseGlaresReset()
{
    glare_mgr.init();
}

#ifndef NON_MATCHING
void xScrFX_PulseGlaresUpdatePos(S32, xVec3* pos)
{
    xASSERT(0, pos);
}

void xScrFX_PulseGlaresUpdateDir(S32, xVec3* dir)
{
    xASSERT(0, dir);
}

void xScrFX_PulseGlaresUpdateSize(S32, F32 size)
{
    xASSERT(0, size > 0.0f);
}

void xScrFX_PulseGlaresSetScale(S32, F32 scale)
{
    xASSERT(0, scale > 0.0f);
}

void xScrFX_PulseGlaresSetLifetime(S32, F32 lifetime)
{
    xASSERT(0, lifetime > 0.0f);
}

void xScrFX_PulseGlaresSetFlag(S32 flag, S32, bool)
{
    xASSERT(0, flag > 0);
}
#endif
