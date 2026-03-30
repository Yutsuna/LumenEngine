/**
 * @file Vec.hpp
 * @brief Declaration of the Vec struct for mathematical vector operations.
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

        template <typename Type, USize Dimension> struct TVecData
        {
            Type Data[Dimension] = {};
        };

#if defined( LUMEN_ENGINE_COMPILER_CLANG )
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wc11-extensions"
    #pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
    #pragma clang diagnostic ignored "-Wnested-anon-types"

#elif defined( LUMEN_ENGINE_COMPILER_GCC )
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"

#endif

        template <typename Type> struct TVecData<Type, 2>
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

        template <typename Type> struct TVecData<Type, 3>
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

        template <typename Type> struct TVecData<Type, 4>
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
    concept CVecDimension = Dimension > 0 && Dimension <= 4;

    template <typename Type, USize Dimension>
        requires CVecDimension<Type, Dimension>
    struct TVec : public Private::TVecData<Type, Dimension>
    {
        constexpr TVec () = default;
        constexpr TVec ( const Type &Value );
        constexpr TVec ( const TVec<Type, Dimension> &Other ) = default;

        template <typename... Arguments>
            requires( sizeof...( Arguments ) == Dimension && ( std::is_convertible_v<Arguments, Type> && ... ) )
        constexpr TVec( Arguments &&...Args );
    };

    using FVec2f = TVec<Float32, 2>;
    using FVec3f = TVec<Float32, 3>;
    using FVec4f = TVec<Float32, 4>;

    using FVec2d = TVec<Float64, 2>;
    using FVec3d = TVec<Float64, 3>;
    using FVec4d = TVec<Float64, 4>;

    using FVec2i = TVec<Int32, 2>;
    using FVec3i = TVec<Int32, 3>;
    using FVec4i = TVec<Int32, 4>;

    using FVec2u = TVec<UInt32, 2>;
    using FVec3u = TVec<UInt32, 3>;
    using FVec4u = TVec<UInt32, 4>;

} // namespace Maths

} // namespace LumenEngine

#include "Inline/Vec.inl"
