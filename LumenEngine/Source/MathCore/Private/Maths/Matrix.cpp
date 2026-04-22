/**
 * @file Matrix.cpp
 * @brief Implementation of the Matrix struct.
 */

#include "Maths/Matrix.hpp"

void LumenEngine::Maths::TransformAABB ( const FVec3f &LocalMin, const FVec3f &LocalMax, const FMatrix4x4f &Transform, FVec3f &OutWorldMin, FVec3f &OutWorldMax ) noexcept
{
    const FVec3f T{ Transform[3].Data[0], Transform[3].Data[1], Transform[3].Data[2] };

    OutWorldMin = T;
    OutWorldMax = T;

    /** INFO: Use Arvo's method to compute the transformed AABB */
    for ( USize Col = 0; Col < 3; ++Col )
    {
        const Float32 A = Transform[Col].Data[0] * LocalMin.Data[Col];
        const Float32 B = Transform[Col].Data[0] * LocalMax.Data[Col];
        OutWorldMin.Data[0] += ( A < B ) ? A : B;
        OutWorldMax.Data[0] += ( A < B ) ? B : A;

        const Float32 C = Transform[Col].Data[1] * LocalMin.Data[Col];
        const Float32 D = Transform[Col].Data[1] * LocalMax.Data[Col];
        OutWorldMin.Data[1] += ( C < D ) ? C : D;
        OutWorldMax.Data[1] += ( C < D ) ? D : C;

        const Float32 E = Transform[Col].Data[2] * LocalMin.Data[Col];
        const Float32 F = Transform[Col].Data[2] * LocalMax.Data[Col];
        OutWorldMin.Data[2] += ( E < F ) ? E : F;
        OutWorldMax.Data[2] += ( E < F ) ? F : E;
    }
}