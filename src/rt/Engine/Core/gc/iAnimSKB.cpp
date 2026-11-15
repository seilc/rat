#include "xCommon.h"
#include "iAnimSKB.h"

#include "xMath.h"
#include "iMath.h"

#include "decomp.h"

#include <cmath>
#include <rtslerp.h>

struct SKAKey {
    F32 quat[4];
    F32 tran[3];
    F32 time;
    U32 prevFrame;
};

struct SKAHeader {
    U32 numFrames;
    U32 flags;
    F32 duration;
};

struct ANMHeader {
    U32 chunkType;
    U32 chunkLength;
    U32 chunkVersion;
    S32 hanimVersion;
    S32 hanimTypeID;
    U32 numFrames;
    S32 flags;
    F32 duration;
};

struct ANMKey {
    F32 time;
    F32 quat[4];
    F32 tran[3];
    U32 prevFrame;
};

namespace {
inline F32 QUAT_DECOMPRESS(S16 x, S16 y, S16 z, F32 scale)
{
    return xsqrt(std::fabsf(1.0f - scale * (x * x) - scale * (y * y) - scale * (z * z)));
}
} // namespace

void iAnimEvalSKB(iAnimSKBHeader* data, F32 time, U32 flags, xVec3* tran, xQuat* quat)
{
#if DEBUG
    xASSERT_L(423, tran);
    xASSERT_L(424, quat);
#endif

    U32 i, tidx, tranCount = data->TranCount, bcount = data->BoneCount, tcount = data->TimeCount;
    iAnimSKBKey* keys = (iAnimSKBKey*)(data + 1);
    U16* times = (U16*)(keys + data->KeyCount);
    S16* tran1 = (S16*)(times + ((tcount + 1) & ~1));
    U16* offsets = (U16*)(tran1 + ((tranCount * 3 + 1) & ~1));

    if (time < 0.0f) time = 0.0f;

    F32 timeX = 30.0f * time;
    if (timeX > times[tcount - 1]) {
        timeX = times[tcount - 1];
    }

    tidx = (tcount - 1) & 3;
    while (times[tidx] < timeX)
        tidx += 4;
    while (tidx && timeX <= times[tidx])
        tidx--;

    offsets += tidx * bcount;

    if (flags & 0x1) {
        bcount = 1;
    }
    if (flags & 0x2) {
        bcount--;
        offsets++;
    }

    F32 scalex = data->Scale[0];
    F32 scaley = data->Scale[1];
    F32 scalez = data->Scale[2];

    if (tcount == 1) {
#if DEBUG
        xASSERT_L(527, data->KeyCount==data->BoneCount*2);
#endif

        F32 scalex = data->Scale[0];
        F32 scaley = data->Scale[1];
        F32 scalez = data->Scale[2];

        for (i = 0; i < bcount; i++) {
            iAnimSKBKey* k = &keys[i * 2];

            quat->v.x = (1.0f / 32767.0f) * k->Quat[0];
            quat->v.y = (1.0f / 32767.0f) * k->Quat[1];
            quat->v.z = (1.0f / 32767.0f) * k->Quat[2];
            quat->s = QUAT_DECOMPRESS(k->Quat[0], k->Quat[1], k->Quat[2], 1.0f / (32767.0f * 32767.0f));
            quat->s = (k->Frame & 0x8000) ? -quat->s : quat->s;

            S16* tmpTran = tran1 + k->TranIndex * 3;
            tran->x = scalex * tmpTran[0];
            tran->y = scaley * tmpTran[1];
            tran->z = scalez * tmpTran[2];

            quat++;
            tran++;
        }
    } else {
        F32 scalex = data->Scale[0];
        F32 scaley = data->Scale[1];
        F32 scalez = data->Scale[2];
        F32 time1, time2, lerp;

        for (i = 0; i < bcount; i++) {
            iAnimSKBKey* k = &keys[*offsets];
            offsets++;

            time1 = (k[0].Frame & 0x8000) ? (k[0].Frame & ~0x8000) : k[0].Frame;
            time2 = (k[1].Frame & 0x8000) ? (k[1].Frame & ~0x8000) : k[1].Frame;
            lerp = (timeX - time1) / (time2 - time1);

            RtQuat q1, q2;
            RtQuatSlerpCache qcache;

            q1.imag.x = (1.0f / 32767.0f) * k[0].Quat[0];
            q1.imag.y = (1.0f / 32767.0f) * k[0].Quat[1];
            q1.imag.z = (1.0f / 32767.0f) * k[0].Quat[2];
            q1.real = QUAT_DECOMPRESS(k[0].Quat[0], k[0].Quat[1], k[0].Quat[2], 1.0f / (32767.0f * 32767.0f));
            q1.real = (k[0].Frame & 0x8000) ? -q1.real : q1.real;

            q2.imag.x = (1.0f / 32767.0f) * k[1].Quat[0];
            q2.imag.y = (1.0f / 32767.0f) * k[1].Quat[1];
            q2.imag.z = (1.0f / 32767.0f) * k[1].Quat[2];
            q2.real = QUAT_DECOMPRESS(k[1].Quat[0], k[1].Quat[1], k[1].Quat[2], 1.0f / (32767.0f * 32767.0f));
            q2.real = (k[1].Frame & 0x8000) ? -q2.real : q2.real;

            RtQuatSetupSlerpCache(&q1, &q2, &qcache);
            RtQuatSlerp((RtQuat*)quat, &q1, &q2, lerp, &qcache);

            xVec3 t1, t2;
            S16* tmpTran0 = tran1 + k[0].TranIndex * 3, *tmpTran1 = tran1 + k[1].TranIndex * 3;

            t1.x = scalex * tmpTran0[0];
            t1.y = scaley * tmpTran0[1];
            t1.z = scalez * tmpTran0[2];

            t2.x = scalex * tmpTran1[0];
            t2.y = scaley * tmpTran1[1];
            t2.z = scalez * tmpTran1[2];

            tran->x = t1.x + lerp * (t2.x - t1.x);
            tran->y = t1.y + lerp * (t2.y - t1.y);
            tran->z = t1.z + lerp * (t2.z - t1.z);

            quat++;
            tran++;
        }
    }
}

F32 iAnimDurationSKB(iAnimSKBHeader* data)
{
    return ((U16*)((iAnimSKBKey*)(data + 1) + data->KeyCount))[data->TimeCount - 1] / 30.0f;
}

static S32 CmpTime(const void* a, const void* b); // UNUSED
static S32 CmpBoneTime(const void* a, const void* b); // UNUSED
static void* ConvertCommon(SKAKey* copykey, U32 keyCount, U32* size); // UNUSED

DECOMP_FORCEFLOAT(32767.0f)

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("iAnimSKB.cpp", "finalData", "%s", "keyIndex < keyCount", "key[keyIndex].Frame == 0", "copykey",
                   "xMemMgr.h", "ptr", DEBUG ? "for %d bytes from %s, in %s line %d" : 0,
                   "anmhead->chunkType == 0x1B", "anmhead->hanimVersion == 0x100", "anmhead->hanimTypeID == 0x1",
                   "Keys:  %d\n", "%d %f %f %f %f %f %f %f %f\n", "keyIndex < kcount", "keys[keyIndex].Frame == 0")
#endif

void _iAnimSKBAdjustTranslate(iAnimSKBHeader* data, U32 bone, F32* starttran, F32* endtran)
{
    S32 ipos;
    U32 i, idx, keyfirst, keylast, tranCount = data->TranCount, kcount = data->KeyCount, bcount = data->BoneCount,
        tcount = data->TimeCount;
    F32 outScale[3], pos, factor[3], oldmax[3] = { 0.0f, 0.0f, 0.0f }, newmax[3] = { 0.0f, 0.0f, 0.0f }, timefirst,
        timelast;
    iAnimSKBKey* keys = (iAnimSKBKey*)(data + 1);
    S16* times = (S16*)(keys + kcount);
    S16* tran = times + ((tcount + 1) & ~1);
    U16* offsets = (U16*)(tran + ((tranCount * 3 + 1) & ~1));

    xASSERT_L(1446, bone < bcount);

    keyfirst = offsets[bone];
    keylast = offsets[bone + (tcount - 2) * bcount] + 1;

    timefirst = times[0];
    timelast = times[tcount - 1];

    for (i = 0; i < kcount; i++)
        for (idx = 0; idx < 3; idx++)
            if (starttran[idx] || endtran[idx]) {
                S16* tmpTran = tran + keys[i].TranIndex * 3;
                pos = data->Scale[idx] * tmpTran[idx];

                if (iabs(pos) > oldmax[idx]) oldmax[idx] = iabs(pos);
                if (i >= keyfirst && i <= keylast) {
                    pos += starttran[idx] + (endtran[idx] - starttran[idx]) *
                        (((keys[i].Frame & 0x8000) ? (F32)(keys[i].Frame & ~0x8000) : (F32)keys[i].Frame) - timefirst) / (timelast - timefirst);
                    if (iabs(pos) > newmax[idx]) newmax[idx] = iabs(pos);
                }
            }

    for (idx = 0; idx < 3; idx++)
        if (starttran[idx] || endtran[idx]) {
            if (newmax[idx] > oldmax[idx])
                outScale[idx] = data->Scale[idx] * newmax[idx] / oldmax[idx];
            else
                outScale[idx] = data->Scale[idx];
            factor[idx] = 1.0f / outScale[idx];
        }

    for (i = 0; i < kcount; i++)
        for (idx = 0; idx < 3; idx++)
            if (starttran[idx] || endtran[idx]) {
                if (i >= keyfirst && i <= keylast) {
                    S16* tmpTran = tran + keys[i].TranIndex * 3;
                    ipos = factor[idx] * (data->Scale[idx] * tmpTran[idx] +
                        (starttran[idx] + (endtran[idx] - starttran[idx]) *
                        (((keys[i].Frame & 0x8000) ? (F32)(keys[i].Frame & ~0x8000) : (F32)keys[i].Frame) - timefirst) / (timelast - timefirst)));

                    if (ipos < -32767) ipos = -32767;
                    else if (ipos > 32767) ipos = 32767;

                    tmpTran[idx] = ipos;
                } else if (data->Scale[idx] != outScale[idx]) {
                    S16* tmpTran = tran + keys[i].TranIndex * 3;
                    ipos = data->Scale[idx] * tmpTran[idx] * factor[idx];

                    if (ipos < -32767) ipos = -32767;
                    else if (ipos > 32767) ipos = 32767;

                    tmpTran[idx] = ipos;
                }
            }

    for (idx = 0; idx < 3; idx++)
        if (starttran[idx] || endtran[idx]) {
            data->Scale[idx] = outScale[idx];
        }
}

S32 _iAnimSKBNumExtract(iAnimSKBHeader* data, U32 bone)
{
    U32 i, keyfirst, keylast, tranCount = data->TranCount, kcount = data->KeyCount, bcount = data->BoneCount,
        tcount = data->TimeCount;
    iAnimSKBKey* keys = (iAnimSKBKey*)(data + 1);
    S16* times = (S16*)(keys + kcount);
    S16* tran = times + ((tcount + 1) & ~1);
    U16* offsets = (U16*)(tran + ((tranCount * 3 + 1) & ~1));
    S32 tranFound = 0, lastTime, currTime;

    xASSERT_L(1555, bone < bcount);

    keyfirst = offsets[bone];
    keylast = offsets[bone + (tcount - 2) * bcount] + 1;

    lastTime = -1;
    for (i = keyfirst; i <= keylast; i++) {
        currTime = (keys[i].Frame & 0x8000) ? (keys[i].Frame & ~0x8000) : keys[i].Frame;

        if (lastTime >= 0 && currTime > lastTime + 1) {
            tranFound += currTime - (lastTime + 1);
        }

        if (lastTime != currTime) {
            tranFound++;
        }

        lastTime = currTime;
    }

    return tranFound;
}

S32 _iAnimSKBExtractTranslateAxis(iAnimSKBHeader* data, U32 bone, xVec3* tranArray, S32 tranCount, U8* axis)
{
    U32 i, j, keyfirst, keylast, traCount = data->TranCount, kcount = data->KeyCount, bcount = data->BoneCount,
        tcount = data->TimeCount;
    iAnimSKBKey* keys = (iAnimSKBKey*)(data + 1);
    S16* times = (S16*)(keys + kcount);
    S16* tran = times + ((tcount + 1) & ~1);
    U16* offsets = (U16*)(tran + ((traCount * 3 + 1) & ~1));
    xVec3 currTran;
    xVec3* lastTran;
    S32 tranFound = 0, lastTime, currTime;

    xASSERT_L(1632, bone < bcount);

    keyfirst = offsets[bone];
    keylast = offsets[bone + (tcount - 2) * bcount] + 1;

    lastTime = -1;
    for (i = keyfirst; i <= keylast; i++) {
        currTime = (keys[i].Frame & 0x8000) ? (keys[i].Frame & ~0x8000) : keys[i].Frame;

        S16* tmpTran = tran + keys[i].TranIndex * 3;

        if (axis[0])
            currTran.x = tmpTran[0] * data->Scale[0];
        else
            currTran.x = 0.0f;

        if (axis[1])
            currTran.y = tmpTran[1] * data->Scale[1];
        else
            currTran.y = 0.0f;

        if (axis[2])
            currTran.z = tmpTran[2] * data->Scale[2];
        else
            currTran.z = 0.0f;

        if (lastTime >= 0 && currTime > lastTime + 1) {
            lastTran = tranArray - 1;
            for (j = 1; j < currTime - lastTime; j++) {
                F32 lerp;

                xASSERT_L(1691, tranCount);
                lerp = (F32)j / (F32)(currTime - lastTime);
                tranArray->x = lastTran->x + lerp * (currTran.x - lastTran->x);
                tranArray->y = lastTran->y + lerp * (currTran.y - lastTran->y);
                tranArray->z = lastTran->z + lerp * (currTran.z - lastTran->z);
                tranArray++;
                tranCount--;
                tranFound++;
            }
        }

        if (lastTime != currTime) {
            xASSERT_L(1707, tranCount);
            tranArray->x = currTran.x;
            tranArray->y = currTran.y;
            tranArray->z = currTran.z;
            tranArray++;
            tranCount--;
            tranFound++;
        }

        lastTime = currTime;

        for (j = 0; j < 3; j++) {
            if (axis[j]) {
                tmpTran[j] = 0;
            }
        }
    }

    return tranFound;
}

F32 QuatToYaw(xQuat* q)
{
    F32 yaw, x, y;

    x = 2.0f * (q->s * q->v.y - q->v.x * q->v.z);
    // BUG: z + z, not z * z
    y = q->s * q->s + q->v.x * q->v.x - q->v.y * q->v.y - (q->v.z + q->v.z);

    yaw = xatan2(x, y);
    return yaw;
}

void YawToQuat(F32 yaw, xQuat* q)
{
    q->s = icos(0.5f * yaw);
    q->v.x = 0.0f;
    q->v.y = isin(0.5f * yaw);
    q->v.z = 0.0f;
}

static void QuatMul(xQuat* o, xQuat* a, xQuat* b)
{
    F32 s = a->s * b->s - a->v.x * b->v.x - a->v.y * b->v.y - a->v.z * b->v.z,
        x = a->s * b->v.x + a->v.x * b->s + a->v.y * b->v.z - a->v.z * b->v.y,
        y = a->s * b->v.y + a->v.y * b->s + a->v.z * b->v.x - a->v.x * b->v.z,
        z = a->s * b->v.z + a->v.z * b->s + a->v.x * b->v.y - a->v.y * b->v.x;

    o->v.x = x;
    o->v.y = y;
    o->v.z = z;
    o->s = s;
}

S32 _iAnimSKBExtractYaw(iAnimSKBHeader* data, U32 bone, F32* yawArray, S32 yawCount)
{
    U32 i, j, keyfirst, keylast, tranCount = data->TranCount, kcount = data->KeyCount, bcount = data->BoneCount,
        tcount = data->TimeCount;
    iAnimSKBKey* keys = (iAnimSKBKey*)(data + 1);
    S16* times = (S16*)(keys + kcount);
    S16* tran = times + ((tcount + 1) & ~1);
    U16* offsets = (U16*)(tran + ((tranCount * 3 + 1) & ~1));
    xQuat currQuat;
    F32 currYaw, lastYaw, lastCurrYaw = PI;
    S32 yawFound = 0, lastTime, currTime;

    xASSERT_L(1805, bone < bcount);

    keyfirst = offsets[bone];
    keylast = offsets[bone + (tcount - 2) * bcount] + 1;

    lastTime = -1;
    for (i = keyfirst; i <= keylast; i++) {
        currTime = (keys[i].Frame & 0x8000) ? (keys[i].Frame & ~0x8000) : keys[i].Frame;

        currQuat.v.x = (1.0f / 32767.0f) * keys[i].Quat[0];
        currQuat.v.y = (1.0f / 32767.0f) * keys[i].Quat[1];
        currQuat.v.z = (1.0f / 32767.0f) * keys[i].Quat[2];
        currQuat.s = QUAT_DECOMPRESS(keys[i].Quat[0], keys[i].Quat[1], keys[i].Quat[2], 1.0f / (32767.0f * 32767.0f));
        currQuat.s = (keys[i].Frame & 0x8000) ? -currQuat.s : currQuat.s;

        currYaw = QuatToYaw(&currQuat);

        while (currYaw + PI < lastCurrYaw) {
            currYaw += 2.0f * PI;
        }
        while (currYaw - PI > lastCurrYaw) {
            currYaw -= 2.0f * PI;
        }

        if (lastTime >= 0 && currTime > lastTime + 1) {
            lastYaw = yawArray[-1];
            for (j = 1; j < currTime - lastTime; j++) {
                F32 lerp;

                xASSERT_L(1851, yawCount);
                lerp = (F32)j / (F32)(currTime - lastTime);
                *yawArray = lastYaw + lerp * (currYaw - lastYaw);
                yawArray++;
                yawCount--;
                yawFound++;
            }
        }

        if (lastTime != currTime) {
            xASSERT_L(1865, yawCount);
            *yawArray = currYaw;
            yawArray++;
            yawCount--;
            yawFound++;
        }

        lastTime = currTime;

        xQuat antiYaw;
        YawToQuat(-currYaw, &antiYaw);
        QuatMul(&currQuat, &currQuat, &antiYaw);
        keys[i].Quat[0] = CLAMP((S32)(32767.0f * currQuat.v.x), -32767.0f, 32767.0f);
        keys[i].Quat[1] = CLAMP((S32)(32767.0f * currQuat.v.y), -32767.0f, 32767.0f);
        keys[i].Quat[2] = CLAMP((S32)(32767.0f * currQuat.v.z), -32767.0f, 32767.0f);

        lastCurrYaw = currYaw;
    }

    return yawFound;
}

void _iAnimSKBAdjustYaw(iAnimSKBHeader* data, U32 bone, F32 yawStart, F32 yawEnd)
{
    U32 i, keyfirst, keylast, tranCount = data->TranCount, kcount = data->KeyCount, bcount = data->BoneCount,
        tcount = data->TimeCount;
    F32 yaw;
    xQuat quatA, quatB;
    F32 timefirst, timelast;
    iAnimSKBKey* keys = (iAnimSKBKey*)(data + 1);
    S16* times = (S16*)(keys + kcount);
    S16* tran = times + ((tcount + 1) & ~1);
    U16* offsets = (U16*)(tran + ((tranCount * 3 + 1) & ~1));

    xASSERT_L(1915, bone < bcount);

    keyfirst = offsets[bone];
    keylast = offsets[bone + (tcount - 2) * bcount] + 1;
    timefirst = times[0];
    timelast = times[tcount - 1];
    F32 invTimeDiff = 1.0f / (timelast - timefirst);

    for (i = 0; i < kcount; i++) {
        if (i >= keyfirst && i <= keylast) {
            quatA.v.x = (1.0f / 32767.0f) * keys[i].Quat[0];
            quatA.v.y = (1.0f / 32767.0f) * keys[i].Quat[1];
            quatA.v.z = (1.0f / 32767.0f) * keys[i].Quat[2];
            quatA.s = QUAT_DECOMPRESS(keys[i].Quat[0], keys[i].Quat[1], keys[i].Quat[2], 1.0f / (32767.0f * 32767.0f));
            quatA.s = (keys[i].Frame & 0x8000) ? -quatA.s : quatA.s;

            yaw = yawStart + invTimeDiff * ((yawEnd - yawStart) *
                (((keys[i].Frame & 0x8000) ? (keys[i].Frame & ~0x8000) : keys[i].Frame) - timefirst));

            YawToQuat(yaw, &quatB);
            QuatMul(&quatA, &quatA, &quatB);

            keys[i].Quat[0] = CLAMP((S32)(32767.0f * quatA.v.x), -32767.0f, 32767.0f);
            keys[i].Quat[1] = CLAMP((S32)(32767.0f * quatA.v.y), -32767.0f, 32767.0f);
            keys[i].Quat[2] = CLAMP((S32)(32767.0f * quatA.v.z), -32767.0f, 32767.0f);
        }
    }
}
