/**
 * @file Vector.hpp
 * @brief Declaration of the Vector struct for mathematical vector operations.
 */

#pragma once

#include "CoreTypes.hpp"

#include <type_traits>

namespace LumenEngine
{

namespace Math
{

    namespace Private
    {

        template <typename Type, USize Dimension>
        struct TVectorData
        {
            Type Data[Dimension] = {};
        };

        template <typename Type>
        struct TVectorData<Type, 2>
        {
            union
            {
                struct
                {
                    Type x, y;
                };
                struct
                {
                    Type r, g;
                };
                struct
                {
                    Type s, t;
                };
                struct
                {
                    Type width, height;
                };
                Type Data[2] = {};
            };
        };

        template <typename Type>
        struct TVectorData<Type, 3>
        {
            union
            {
                struct
                {
                    Type x, y, z;
                };
                struct
                {
                    Type r, g, b;
                };
                struct
                {
                    Type s, t, p;
                };
                Type Data[3] = {};
            };
        };

        template <typename Type>
        struct TVectorData<Type, 4>
        {
            union
            {
                struct
                {
                    Type x, y, z, w;
                };
                struct
                {
                    Type r, g, b, a;
                };
                struct
                {
                    Type s, t, p, q;
                };
                Type Data[4] = {};
            };
        };

    } // namespace Private

    template <typename Type, USize Dimension>
    concept CVectorDimension =
        Dimension > 0 && Dimension <= 4;

    template <typename Type, USize Dimension>
        requires CVectorDimension<Type, Dimension>
    struct TVector : public Private::TVectorData<Type, Dimension>
    {
        constexpr TVector () = default;
        constexpr TVector ( const Type &Value );
        constexpr TVector ( const TVector<Type, Dimension> &Other ) = default;

        template <typename... Arguments>
            requires( sizeof...( Arguments ) == Dimension && ( std::is_convertible_v<Arguments, Type> && ... ) )
        constexpr TVector( Arguments &&...Args );
    };

    using FVec2f = TVector<Float32, 2>;
    using FVec3f = TVector<Float32, 3>;
    using FVec4f = TVector<Float32, 4>;

    using FVec2d = TVector<Float64, 2>;
    using FVec3d = TVector<Float64, 3>;
    using FVec4d = TVector<Float64, 4>;

    using FVec2i = TVector<Int32, 2>;
    using FVec3i = TVector<Int32, 3>;
    using FVec4i = TVector<Int32, 4>;

    using FVec2u = TVector<UInt32, 2>;
    using FVec3u = TVector<UInt32, 3>;
    using FVec4u = TVector<UInt32, 4>;

} // namespace Math

} // namespace LumenEngine

#include "Inline/Vector.inl"
