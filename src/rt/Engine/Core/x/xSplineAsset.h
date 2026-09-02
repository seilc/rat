#ifndef XSPLINEASSET_H
#define XSPLINEASSET_H

#include "xBaseAsset.h"
#include "xMath3.h"

class xNurbs : public xBaseAsset
{
private:
    S32 degree;
    S32 knotMaxIndex;
    S32 controlMaxIndex;
    F32* knot;
    xVec3* control;

public:
    F32 approximate_length(S32 divisions) const;
    S32 find_span(F32 u) const;
    void find_basis_functions(F32 u, S32 i, F32* N) const;
    void find_derivative_basis_functions(F32 u, S32 i, S32 order, F32 ders[5][5]) const;
    F32 start() const;
    F32 end() const;
    void evaluate(F32 u, xVec3& point) const;
    xVec3 evaluate(F32 u, S32 d) const;
    void evaluate(F32 u, S32 d, xVec3& point) const;
    xVec3 project_point(const xVec3& point, F32* u, F32* distance, F32 guess) const;
    void getBoundBox(xBox& box) const;
    void AddCrossSectionPos(xVec3& lastPos, xVec3& centerLinePos, F32 halfWidth) const;
    void DrawCrossSection(S32 segments, F32 width) const;
    bool advance_u(F32 start_u, F32 distance, bool forward, F32& new_u) const;
};

#endif
