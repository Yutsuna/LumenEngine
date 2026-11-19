/**
 * @file IsIntegral.hpp
 * @brief Integral type trait and concept for Lumen.
 */

#pragma once

#include <Core/CoreTypes.hpp>

namespace Lumen
{

namespace Internal
{

    template <class TObjectType> struct TRemoveCv
    {
        using Type = TObjectType;
    };

    template <class TObjectType> struct TRemoveCv<const TObjectType>
    {
        using Type = TObjectType;
    };

    template <class TObjectType> struct TRemoveCv<volatile TObjectType>
    {
        using Type = TObjectType;
    };

    template <class TObjectType> struct TRemoveCv<const volatile TObjectType>
    {
        using Type = TObjectType;
    };

    template <class TObjectType> inline constexpr Bool TIsIntegralImplV = false;

    template <> inline constexpr Bool TIsIntegralImplV<Bool>     = true;
    template <> inline constexpr Bool TIsIntegralImplV<AnsiChar> = true;
    template <> inline constexpr Bool TIsIntegralImplV<Int8>     = true;
    template <> inline constexpr Bool TIsIntegralImplV<UInt8>    = true;
    template <> inline constexpr Bool TIsIntegralImplV<WideChar> = true;
    template <> inline constexpr Bool TIsIntegralImplV<Int16>    = true;
    template <> inline constexpr Bool TIsIntegralImplV<UInt16>   = true;
    template <> inline constexpr Bool TIsIntegralImplV<Int32>    = true;
    template <> inline constexpr Bool TIsIntegralImplV<UInt32>   = true;
    template <> inline constexpr Bool TIsIntegralImplV<Int64>    = true;
    template <> inline constexpr Bool TIsIntegralImplV<UInt64>   = true;

} // namespace Internal

namespace Concepts
{

    template <class Type>
    inline constexpr Bool TIsIntegralT = Internal::TIsIntegralImplV<typename Internal::TRemoveCv<Type>::Type>;

    /**
     * @concept CIsIntegral
     * @brief Concept for integral types.
     */
    template <class Type>
    concept CIsIntegral = TIsIntegralT<Type>;

} // namespace Concepts

} // namespace Lumen
