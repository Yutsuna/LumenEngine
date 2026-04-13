/**
 * @file SIMDTypes.hpp
 * @brief Declaration of SIMD types and utilities for mathematical operations.
 */

#pragma once

#include "CoreTypes.hpp"

#include <type_traits>

namespace LumenEngine
{

namespace Maths
{

    namespace SIMD
    {

        namespace Concepts
        {

            template <typename Type>
            concept CSIMDType = std::is_same_v<Type, Float32> or std::is_same_v<Type, Int32>;

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

    } // namespace SIMD

} // namespace Maths

} // namespace LumenEngine