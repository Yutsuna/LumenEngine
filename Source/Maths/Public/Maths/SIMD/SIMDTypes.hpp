/**
 * @file SIMDTypes.hpp
 * @brief Declaration of SIMD types and utilities for mathematical operations.
 */

#pragma once

#include <Concepts/SameAs.hpp>
#include <Core/CoreTypes.hpp>

namespace Lumen
{

namespace Maths
{

    namespace SIMD
    {

        namespace Concepts
        {

            template <typename Type>
            concept CSIMDType = ::Lumen::Concepts::CSameAs<Type, Float32> or ::Lumen::Concepts::CSameAs<Type, Int32>;

        } // namespace Concepts

        /**
         * @struct TSimdRegister
         * @brief A struct representing a SIMD register for a 128-bit vector (4x32 bits)
         * @tparam Type The type of the SIMD register.
         */
        template <typename Type> struct alignas( 16 ) TSimdRegister
        {
            using ValueType = Type;
        };

        using FSimdRegisterF32 = TSimdRegister<Float32>;
        using FSimdRegisterI32 = TSimdRegister<Int32>;

        /**
         * @struct FGenericFloat32x4
         * @brief Fallback structure for platforms without native SIMD support.
         */
        struct alignas( 16 ) FGenericFloat32x4 final : public TSimdRegister<Float32>
        {
            Float32 Data[4];
        };

    } // namespace SIMD

} // namespace Maths

} // namespace Lumen
