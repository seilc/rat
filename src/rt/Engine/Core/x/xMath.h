#ifndef XMATH_H
#define XMATH_H

#include "iMath.h"
#include "xDebug.h"

#include <cmath>

#define EPSILON 1e-5f
#define PI 3.14159265358979323846f
#define HUGE 1e38f

#define xabs(x) iabs(x)
#define xeq(a,b,e) (xabs((a) - (b)) <= (e))
#define xfeq0(x) (((x) >= -EPSILON) && ((x) <= EPSILON))
#define xmin(a,b) ((a) < (b) ? (a) : (b))
#define xmax(a,b) ((a) > (b) ? (a) : (b))
#define xmax3(a,b,c) (xmax(xmax((a), (b)), (c)))
#define xclamp(x,a,b) xmax((a), xmin((x), (b)))
#define xsqr(x) ((x) * (x))
#define xsignbit(x) ((x) < 0.0f ? 1 : 0)
#define xsamesign(a,b) (xsignbit(a) == xsignbit(b))

inline S32 log2ceil(S32 value)
{
    S32 power = 0;

    S32 v = value;
    while (v > 1) {
        v >>= 1;
        power++;
    }

    if ((1 << power) < value) {
        power++;
    }

    return power;
}

template <class T>
T range_limit(T v, T minv, T maxv);

struct tweak_callback;
struct tweak_info;

struct xFuncPiece
{
    F32 coef[5];
    F32 end;
    S32 order;
    xFuncPiece* next;
};

struct xResponseCurve
{
    struct node
    {
        F32 t;
    };

    struct inode : node
    {
        F32 value[1];
    };

#if DEBUG || RELEASE
    struct debug_data;
#endif

    void reset(U32 values, const void* curve, U32 nodes);
    void eval_linear(F32 t, F32* value) const;
    void eval_smooth(F32 t, F32* value) const;
#if DEBUG || RELEASE
    bool sorted() const;
    void add_tweaks(const char* tweak_prefix, const char** value_names, const tweak_callback* cb, void* cb_context);
    void update_node_tweaks();
    void format_curve();
#endif

    inode* get_node(size_t index) const
    {
        return (inode*)((U8*)curve + index * (_values * sizeof(F32) + sizeof(node)));
    }

    bool valid() const
    {
        return curve && _nodes >= 2 && _values;
    }

    U32 values() const
    {
        return _values;
    }

    F32 start_t() const
    {
        xASSERT_L(1007, valid());
        return curve->t;
    }

    F32 end_t() const
    {
        xASSERT_L(1010, valid());
        return get_node(_nodes - 1)->t;
    }

private:
    void find_active_node(F32 t, size_t stride) const
    {
        U32 max_active_node = _nodes - 2;
        const U8* it = (const U8*)curve + stride * active_node;
        while (true) {
            inode& n = *(inode*)it;
            inode& next = *(inode*)(it + stride);
            if (t < n.t) {
                if (active_node == 0) break;
                it -= stride;
                active_node--;
            }
            else if (t > next.t) {
                if (active_node >= max_active_node) break;
                it += stride;
                active_node++;
            } else {
                break;
            }
        }
    }

#if DEBUG || RELEASE
    static void on_change_nodes(const tweak_info& info);
    static void on_change_t(const tweak_info& info);
#endif

    U32 _values;
    const inode* curve;
    U32 _nodes;
    mutable U32 active_node;
#if DEBUG || RELEASE
    debug_data* debug;
#endif
};

void xMathInit();
void xMathExit();
S32 xMathInited();
S32 xMathExited();
void xMathTest();
void xcossin3fast(F32&, F32&, F32&, F32&, F32&, F32&, F32, F32, F32);
S32 xeqtwt(F32 a, F32 b, F32 x);
F32 xquant(F32 x, F32 t);
U32 xMathSolveQuadratic(F32 a, F32 b, F32 c, F32* x1, F32* x2);
U32 xMathSolveCubic(F32 a, F32 b, F32 c, F32 d, F32* x1, F32* x2, F32* x3);
F32 xMathS0(F32 t);
F32 xMathS1(F32 t);
F32 xMathC0(F32 t);
F32 xMathC1(F32 t);
F32 xHump(F32 c, F32 r, F32 x);
F32 xAngleClamp(F32 a);
F32 xAngleClampFast(F32 a);
F32 xDangleClamp(F32 a);
F32 xDangleClampFast(F32 a);
void xAccelMove(F32& x, F32& v, F32 a, F32 dt, F32 endx, F32 maxv);
void xAccelMove2(F32& x, F32& v, F32 decel, F32 accel, F32 dt, F32 endx, F32 maxv);
F32 xAccelMoveTime(F32 dx, F32 a, F32, F32 maxv);
void xAccelMove(F32& x, F32& v, F32 a, F32 dt, F32 maxv);
void xAccelMoveRange(F32& x, F32& v, F32 accel_min, F32 accel_max, F32 dt, F32 endx, F32 maxv);
void xAccelStop(F32& x, F32& v, F32 a, F32 dt);
F32 xFuncPiece_Eval(xFuncPiece* func, F32 param, xFuncPiece** iterator);
void xFuncPiece_Derivative(xFuncPiece* deriv, xFuncPiece* func);
void xFuncPiece_Integral(xFuncPiece* integ, xFuncPiece* func);
void xFuncPiece_EndPoints(xFuncPiece* func, F32 pi, F32 pf, F32 fi, F32 ff);
F32 xFuncPiece_MaxParamNoRoots(F32 fi, F32 ff, F32 vi, F32 vf);
void xFuncPiece_EndPointsAndSlopes(xFuncPiece* func, F32 pi, F32 pf, F32 fi, F32 ff, F32 vi, F32 vf);
void xFuncPiece_EndPointsAllInitial(xFuncPiece* func, F32 pi, F32 pf, F32 fi, F32 ff, F32 vi, F32 ai);
void xFuncPiece_ShiftPiece(xFuncPiece* shift, xFuncPiece* func, F32 newZero);
void xFuncPiece_NewEndpointsPiece(xFuncPiece* move, xFuncPiece* func, F32 oldEnd1, F32 oldEnd2, F32 newEnd1, F32 newEnd2);

F32 xDampSpringCoeff(F32 dt, F32 k);
void xDampSpring(F32& s, F32& v, F32 dt, F32 k, F32 e);
void xDampSpring(F32& s, F32& v, F32 dt, F32 k);

F32 xsqrt(F32 x);
F32 xexp(F32 x);
F32 xasin(F32 x);
F32 xatan(F32 x);
F32 xatan2(F32 y, F32 x);
F32 xfmod(F32 x, F32 y);
void xsqrtfast(F32& o, F32 fVal);

inline F32 xrmod(F32 ang)
{
    F32 frac = (1.0f / (2.0f * PI)) * ang;
    if (frac < 0.0f) return 2.0f * PI * (1.0f + (frac - std::ceilf(frac)));
    else if (frac >= 1.0f) return 2.0f * PI * (frac - std::floorf(frac));
    return ang;
}

#if !DEBUG
#include "xMathInlines.h"
#endif

#endif
