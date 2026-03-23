/**
 * @file Vector.hpp
 * @brief Declaration of the Vector struct for mathematical vector operations.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"

#include <type_traits>

namespace LumenEngine
{

namespace Maths
{

    namespace Private
    {

        template <typename Type, USize Dimension> struct TVectorData
        {
            Type Data[Dimension] = {};
        };

#if defined( LUMEN_ENGINE_COMPILER_CLANG )
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wc11-extensions"

#elif defined( LUMEN_ENGINE_COMPILER_GCC )
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"

#endif

        template <typename Type> struct TVectorData<Type, 2>
        {
            union
            {
                struct
                {
                    Type X, Y;
                };
                struct
                {
                    Type R, G;
                };
                struct
                {
                    Type S, T;
                };
                struct
                {
                    Type Width, Height;
                };
                Type Data[2] = {};
            };
        };

        template <typename Type> struct TVectorData<Type, 3>
        {
            union
            {
                struct
                {
                    Type X, Y, Z;
                };
                struct
                {
                    Type R, G, B;
                };
                struct
                {
                    Type S, T, P;
                };
                Type Data[3] = {};
            };
        };

        template <typename Type> struct TVectorData<Type, 4>
        {
            union
            {
                struct
                {
                    Type X, Y, Z, W;
                };
                struct
                {
                    Type R, G, B, A;
                };
                struct
                {
                    Type S, T, P, Q;
                };
                Type Data[4] = {};
            };
        };

#if defined( LUMEN_ENGINE_COMPILER_CLANG )
    #pragma clang diagnostic pop

#elif defined( LUMEN_ENGINE_COMPILER_GCC )
    #pragma GCC diagnostic pop

#endif

    } // namespace Private

    template <typename Type, USize Dimension>
    concept CVectorDimension = Dimension > 0 && Dimension <= 4;

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

} // namespace Maths

} // namespace LumenEngine

#include "Inline/Vector.inl"
