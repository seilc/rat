#include "xVec3.h"

const xVec3 xVec3::m_Null = { 0, 0, 0 };
const xVec3 xVec3::m_Ones = { 1, 1, 1 };
const xVec3 xVec3::m_UnitAxisX = { 1, 0, 0 };
const xVec3 xVec3::m_UnitAxisY = { 0, 1, 0 };
const xVec3 xVec3::m_UnitAxisZ = { 0, 0, 1 };

F32 xVec3Normalize(xVec3* o, const xVec3* v)
{
    F32 len;
    F32 len2 = v->x * v->x + v->y * v->y + v->z * v->z;
    if (xeq(len2, 1.0f, 0.00001f)) {
        o->x = v->x;
        o->y = v->y;
        o->z = v->z;
        len = 1.0f;
    } else if (xeq(len2, 0.0f, 0.00001f)) {
        o->x = 0.0f;
        o->y = 1.0f;
        o->z = 0.0f;
        len = 0.0f;
    } else {
        len = xsqrt(len2);
        F32 len_inv = 1.0f / len;
        o->x = v->x * len_inv;
        o->y = v->y * len_inv;
        o->z = v->z * len_inv;
    }
    return len;
}

void xVec3Copy(register xVec3* o, register const xVec3* v)
{
    register __vec2x32float__ vxy, vz;
    asm {
        psq_l vxy, 0(v), 0, 0
        psq_l vz, 8(v), 1, 0
        psq_st vxy, 0(o), 0, 0
        psq_st vz, 8(o), 1, 0
    }
}

asm F32 xVec3Dot(register const xVec3* vec1, register const xVec3* vec2)
{
    psq_l f2, 4(vec1), 0, 0
    psq_l f3, 4(vec2), 0, 0
    ps_mul f2, f2, f3
    psq_l f5, 0(vec1), 0, 0
    psq_l f4, 0(vec2), 0, 0
    ps_madd f3, f5, f4, f2
    ps_sum0 f1, f3, f2, f2
}

#include "xVec3Inlines.h"
