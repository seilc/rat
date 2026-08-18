#include "xParSys.h"

#include "iParMgr.h"
#include "xCam.h"
#include "xDebugTweak.h"
#include "xFXHighDynamicRange.h"
#include "xGlobals.h"
#include "xMemMgr.h"
#include "xPtankPool.h"
#include "zEvent.h"
#include "zParSys.h"
#include "zScene.h"

#include "decomp.h"

#include <string.h>

enum ptank_render_mode_enum
{
    PTANK_RENDER_DEFAULT,
    PTANK_RENDER_NONE,
    PTANK_RENDER_ALL,
    MAX_PTANK_RENDER,
    FORCE_INT_PTANK_RENDER = 0xFFFFFFFF
};

struct xParSysInfo
{
    S32 type;
    xParGroupRenderFunction func;
};

DECOMP_FORCEFLOAT(0.0f)
DECOMP_FORCEFLOAT(1.0f)

static const S32 sBlendTable[] = {
    rwBLENDZERO,
    rwBLENDONE,
    rwBLENDSRCCOLOR,
    rwBLENDINVSRCCOLOR,
    rwBLENDSRCALPHA,
    rwBLENDINVSRCALPHA,
    rwBLENDDESTALPHA,
    rwBLENDINVDESTALPHA,
    rwBLENDDESTCOLOR,
    rwBLENDINVDESTCOLOR,
    rwBLENDSRCALPHASAT
};

static ptank_render_mode_enum ptank_render_mode = PTANK_RENDER_ALL;

#if DEBUG || RELEASE
static bool ptank_render_mode_registered = false;
static const char* ptank_render_mode_labels[3] = { "Default", "None", "All" };
#endif

static xVec3 par_offset_right;
static xVec3 par_offset_up;

static void render_par_sprite(void* data, xParGroup* ps);

static xParSysInfo sParSysInfo[] = {
    { eParSysRenderFuncSprite, render_par_sprite },
    { eParSysRenderFuncStreak, iParMgrRenderParSys_Streak },
    { eParSysRenderFuncFlat, iParMgrRenderParSys_Flat },
    { eParSysRenderFuncStatic, iParMgrRenderParSys_Static },
    { eParSysRenderFuncGround, iParMgrRenderParSys_Ground },
    { eParSysRenderFuncQuadStreak, iParMgrRenderParSys_QuadStreak },
    { eParSysRenderFuncInvStreak, iParMgrRenderParSys_InvStreak },
};

DECOMP_FORCEACTIVE(
    "xPTankPool.h",
    "ptank != 0",
    "%s",
    "used <= BLOCK_SIZE"
)

inline bool using_ptank_render(const xParSysAsset& sys)
{
#if DEBUG || RELEASE
    if (!ptank_render_mode_registered) {
        ptank_render_mode_registered = 1;
        xTWEAKSELECT(
            "Particle Emitters|\1Debug|PTank Render Mode",
            (U32*)&ptank_render_mode, ptank_render_mode_labels,
            NULL,
            3,
            NULL, 
            NULL,
            0);
    }
#endif

    return ptank_render_mode == PTANK_RENDER_ALL ||
        (ptank_render_mode == PTANK_RENDER_DEFAULT && (sys.parFlags & 0x80));
}

static void par_sprite_begin()
{
    par_offset_right.Scale(xglobals->cam->mat.right, 0.5f);
    par_offset_up.Scale(xglobals->cam->mat.up, 0.5f);
}

static void par_sprite_update(xParSys& sys, xParGroup& group)
{
    if (!using_ptank_render(*sys.tasset)) {
        return;
    }

    U32 pivot = sys.tasset->parFlags;
    xVec3 offset_right, offset_up;
    if (pivot & 0x8) {
        offset_right = par_offset_right;
    } else if (pivot & 0x20) {
        offset_right.Negate(par_offset_right);
    } else {
        offset_right = 0.0f;
    }
    if (pivot & 0x10) {
        offset_up.Negate(par_offset_up);
    } else if (pivot & 0x40) {
        offset_up = par_offset_up;
    } else {
        offset_up = 0.0f;
    }

    ptank_pool__pos_color_size_uv2 pool;
    pool.rs.texture = sys.txtr_particle;
    pool.rs.src_blend = sBlendTable[sys.tasset->renderSrcBlendMode];
    pool.rs.dst_blend = sBlendTable[sys.tasset->renderDstBlendMode];
    pool.rs.flags = 0;
    pool.reset();

    if (sys.tasset->parFlags & 0x80) {
        pool.rs.flags |= 0x4;
        pool.order_index = sys.tasset->priority;
        pool.order_group = 0x484F424F; // Asset ID?
    }

    S32 min_alpha;
    if (sys.tasset->parFlags & 0x100) {
        pool.rs.flags |= 0x8;
        min_alpha = xFXHighDynamicRangeGetBackgroundGlow();
    } else {
        min_alpha = 0;
    }

    xParCmdTex* tex = group.m_cmdTex;
    for (xPar* p = group.m_root; p != NULL; p = p->m_next) {
        RwSphere worldsph;
        worldsph.center = (RwV3d&)p->m_pos;
        worldsph.radius = p->m_size;
        if (RwCameraFrustumTestSphere(xglobals->screen->icam, &worldsph) == rwSPHEREOUTSIDE) {
            continue;
        }
        
        pool.next();
        if (!pool.valid()) {
            break;
        }

        xVec3& loc = *pool.pos;
        loc = p->m_pos;
        loc.AddScale(offset_up, p->m_size);

        pool.color->r = p->m_c[0];
        pool.color->g = p->m_c[1];
        pool.color->b = p->m_c[2];
        pool.color->a = p->m_c[3] > min_alpha ? p->m_c[3] : (U8)min_alpha;
        pool.size->assign(p->m_size, p->m_size);

        if (tex) {
            pool.uv[0].x = tex->x1 + p->m_texIdx[0] * tex->unit_width;
            pool.uv[0].y = tex->y1 + p->m_texIdx[1] * tex->unit_height;
            pool.uv[1].x = tex->x1 + (p->m_texIdx[0] + 1) * tex->unit_width;
            pool.uv[1].y = tex->y1 + (p->m_texIdx[1] + 1) * tex->unit_height;
        } else {
            pool.uv[0].assign(0.0f, 0.0f);
            pool.uv[1].assign(1.0f, 1.0f);
        }
    }

    pool.flush();
}

static void render_par_sprite(void* data, xParGroup* ps)
{
    if (using_ptank_render(*((zParSys*)data)->tasset)) {
        return;
    }

    iParMgrRenderParSys_Sprite(data, ps);
}

xParGroupRenderFunction xParSysGetRenderFunction(S32 which)
{
    xASSERT(262, (which >= 0) && (which < eParSysRenderFuncCount));

    return sParSysInfo[which].func;
}

void xParCmdTexInit(xParCmdTex* cmd)
{
    xASSERT(270, cmd->cols > 0);
    xASSERT(271, cmd->rows > 0);
    xASSERT(272, cmd->rows <= 16);
    xASSERT(273, cmd->cols <= 16);

    cmd->unit_count = cmd->rows * cmd->cols;
    cmd->unit_width = (cmd->x2 - cmd->x1) / cmd->cols;
    cmd->unit_height = (cmd->y2 - cmd->y1) / cmd->rows;
}

void xParSysInit(void* b, void* tasset)
{
    xParSysInit((xBase*)b, (xParSysAsset*)tasset);
}

void xParSysInit(xBase* b, xParSysAsset* tasset)
{
    xBaseInit(b, tasset);

    xParSys* t = (xParSys*)b;
    t->eventFunc = xParSysEventCB;
    t->tasset = tasset;

    if (t->linkCount > 0) {
        t->link = (xLinkAsset*)((U8*)(t->tasset + 1) + tasset->cmdSize);
    } else {
        t->link = NULL;
    }

    t->visible = tasset->parFlags & 0x1;
    t->cmdCount = tasset->cmdCount;

    U32 size, i;
#if DEBUG || RELEASE
    size = (t->cmdCount <= 8) ? 8 : t->cmdCount;
    t->cmd = (xParCmd*)xMEMALLOC(size * sizeof(xParCmd), 0, eMemMgrTag_Particle, 0, 340);
#else
    size = t->cmdCount;
    if (size > 0) {
        t->cmd = (xParCmd*)xMEMALLOC(size * sizeof(xParCmd), 0, eMemMgrTag_Particle, 0, 0);
    } else {
        t->cmd = NULL;
    }
#endif
    U8* cmdPtr = (U8*)tasset + sizeof(xParSysAsset);
    for (i = 0; i < t->cmdCount; i++) {
        t->cmd[i].flag = 0x1;
        t->cmd[i].tasset = (xParCmdAsset*)cmdPtr;
        cmdPtr += xParCmdGetSize(((xParCmdAsset*)cmdPtr)->type);
    }

#if DEBUG || RELEASE
    {
        U32 size, i;
        size = (t->cmdCount <= 8) ? 8 : t->cmdCount;
        t->cmd_buffer = (xParCmdAny*)xMEMALLOC(size * sizeof(xParCmdAny), 0, eMemMgrTag_Particle, 0, 373);
        for (i = 0; i < t->cmdCount; i++) {
            memcpy(&t->cmd_buffer[i], t->cmd[i].tasset, xParCmdGetSize(t->cmd[i].tasset->type));
            t->cmd[i].tasset = (xParCmdAsset*)&t->cmd_buffer[i];
        }
    }

    for (U32 i = t->cmdCount; i < 8; i++) {
        t->cmd[i].tasset = (xParCmdAsset*)&t->cmd_buffer[i];

        xParCmdMove& cmd = *(xParCmdMove*)t->cmd[i].tasset;
        cmd.type = 0;
        cmd.enabled = true;
        cmd.mode = 0;
        cmd.dir = 0.0f;
    }
#endif
    
    t->group = (xParGroup*)xMEMALLOC(sizeof(xParGroup), 0, eMemMgrTag_Particle, 0, 395);

    xASSERT(397, t->group);

    xParGroupInit(t->group);
    xParGroupSetPriority(t->group, tasset->priority);
    xParGroupRegister(t->group);
    xParGroupSetAging(t->group, ((tasset->parFlags >> 1) & 0x1) == 0);
    xParGroupSetVisibility(t->group, t->visible);
    xParGroupSetBack2Life(t->group, ((tasset->parFlags >> 2) & 0x1) == 0);

    t->parent = NULL;

    for (i = 0; i < t->cmdCount; i++) {
        if (t->cmd[i].tasset->type == 12) {
            t->group->m_cmdTex = (xParCmdTex*)t->cmd[i].tasset;
            xParCmdTexInit(t->group->m_cmdTex);
            break;
        }
    }

    t->group->draw = sParSysInfo[t->tasset->renderFunc].func;
}

void xParSysSetup(xParSys* t)
{
    if (t && t->tasset && t->tasset->parentParSysID != 0) {
        t->parent = (xParSys*)zSceneFindObject(t->tasset->parentParSysID);
    }

    t->txtr_particle = (RwTexture*)xSTFindAsset(t->tasset->textureID, NULL);
}

void xParSysReset(xParSys* t)
{
    xASSERT(452, t);
    xASSERT(453, t->tasset);

    xBaseReset(t, t->tasset);

    if (t->group) {
        xParGroupKillAllParticles(t->group);

        t->visible = t->tasset->parFlags & 0x1;

        xParGroupSetAging(t->group, ((t->tasset->parFlags >> 1) & 0x1) == 0);
        xParGroupSetVisibility(t->group, t->visible);
        xParGroupSetBack2Life(t->group, ((t->tasset->parFlags >> 2) & 0x1) == 0);
    }
}

xBase* xParSysExit(xBase* b, zScene*, void*)
{
    xParSysExit((xParSys*)b);
    return b;
}

void xParSysExit(xParSys* t)
{
    xASSERT(479, t);

    if (t->group) {
        xParGroupKillAllParticles(t->group);
        xParGroupUnregister(t->group);
    }
}

DECOMP_FORCEACTIVE("ent", "s")

void xParSysEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32)
{
    xParSys* t = (xParSys*)to;

    xASSERT(541, to);

    switch (toEvent) {
    case eEventReset:
    case eEventDebugReset:
        xParSysReset(t);
        break;
    case eEventVisible:
        t->visible = true;
        if (t->group) {
            xParGroupSetVisibility(t->group, t->visible);
        }
        break;
    case eEventInvisible:
        t->visible = false;
        if (t->group) {
            xParGroupSetVisibility(t->group, t->visible);
        }
        break;
    case eEventOn:
        if (t->group) {
            xParGroupSetActive(t->group, 1);
        }
        break;
    case eEventOff:
        if (t->group) {
            xParGroupSetActive(t->group, 0);
        }
        break;
    }
}

static void xParGroupUpdateR(xParSys* s, xParGroup* g, F32 dt);

void xParSysUpdate(xBase* to, xScene*, F32 dt)
{
    xASSERT(585, to);

    xParSys* s = (xParSys*)to;
    xParSys* parent = s->parent;

    if (s->tasset->renderFunc == eParSysRenderFuncSprite) {
        par_sprite_begin();
    }

    xASSERT(593, s);

    for (xParGroup* g = s->group; g != NULL; g = g->m_next) {
        if (g->m_active) {
            if (parent) {
                xParGroupUpdateR(parent, g, dt);
            }
            if (g->m_alive) {
                xParGroupUpdate(s, g, dt);
            }
        }

        xParGroupAnimate(g, dt);

        if (g->m_num_of_particles > 0 && s->tasset->renderFunc == eParSysRenderFuncSprite) {
            par_sprite_update(*s, *g);
        }
    }
}

static void xParGroupUpdateR(xParSys* s, xParGroup* g, F32 dt)
{
    xASSERT(629, s);
    xASSERT(630, g);
    xASSERTM(632, s != s->parent, "Particle system has itself as a parent!");
    
    if (s->parent) {
        xParGroupUpdateR(s->parent, g, dt);
    }

    xASSERT(637, s->group);

    if (s->group->m_active) {
        for (U32 i = 0; i < s->cmdCount; i++) {
            xParCmd* cmd = &s->cmd[i];
            xASSERT(645, cmd);
            if (cmd && cmd->tasset) {
                xParCmdUpdateFunction func = xParCmdGetUpdateFunc(cmd->tasset->type);
                if (func) {
                    func(cmd, g, dt);
                }
            }
        }
    }
}

void xParGroupUpdate(xParSys* s, xParGroup* g, F32 dt)
{
    for (U32 i = 0; i < s->cmdCount; i++) {
        xParCmd* cmd = &s->cmd[i];
        xASSERT(664, cmd);
        if (cmd && cmd->tasset) {
            xParCmdUpdateFunction func = xParCmdGetUpdateFunc(cmd->tasset->type);
            if (func) {
                func(cmd, g, dt);
            }
        }
    }
}

#if DEBUG
DECOMP_FORCEACTIVE(
    "SHOW: ALL\n",
    "SHOW: PAR COUNT\n",
    "SHOW: ???\n",
    "b",
    "(%s) par\n",
    "(%s) par %d\n"
)
#endif
