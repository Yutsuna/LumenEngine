/**
 * @file IsVoid.hpp
 * @brief Void type trait and concept for Lumen.
 */

#pragma once

#include <Core/CoreTypes.hpp>

#include <Concepts/IsBool.hpp>

namespace Lumen
{

namespace Internal
{

    template <class TObject> struct TIsVoid : TFalseType
    {
        /* */
    };

    template <> struct TIsVoid<void> : TTrueType
    {
        /* */
    };
    template <> struct TIsVoid<const void> : TTrueType
    {
        /* */
    };
    template <> struct TIsVoid<volatile void> : TTrueType
    {
        /* */
    };
    template <> struct TIsVoid<const volatile void> : TTrueType
    {
        /* */
    };

    template <class TObject> inline constexpr Bool TIsVoidV = TIsVoid<TObject>::Value;

} // namespace Internal

namespace Concepts
{

    /**
     * @concept CIsVoid
     * @brief Checks if a type is void (cv-qualified variants included).
     */
    template <class TObject>
    concept CIsVoid = Internal::TIsVoidV<TObject>;

} // namespace Concepts

} // namespace Lumen
