/**
 * @file IsBool.hpp
 * @brief Boolean type traits and concepts for Lumen.
 */

#pragma once

#include <Core/CoreTypes.hpp>

namespace Lumen
{

namespace Internal
{

    /**
     * @trait TTrueType
     * @brief True value carrier.
     */
    struct TTrueType
    {
        static constexpr Bool Value = true;
        explicit constexpr operator Bool () const noexcept
        {
            return true;
        }
    };

    /**
     * @trait TFalseType
     * @brief False value carrier.
     */
    struct TFalseType
    {
        static constexpr Bool Value = false;
        explicit constexpr operator Bool () const noexcept
        {
            return false;
        }
    };

    template <class TObject> struct TIsBool : TFalseType
    {
        /* */
    };

    template <> struct TIsBool<Bool> : TTrueType
    {
        /* */
    };

    template <> struct TIsBool<const Bool> : TTrueType
    {
        /* */
    };

    template <> struct TIsBool<volatile Bool> : TTrueType
    {
        /* */
    };

    template <> struct TIsBool<const volatile Bool> : TTrueType
    {
        /* */
    };

    template <class TObject> inline constexpr Bool TIsBoolV = TIsBool<TObject>::Value;

    template <Bool Value> struct TBooleanConstant : TFalseType
    {
        /* */
    };

    template <> struct TBooleanConstant<true> : TTrueType
    {
        /* */
    };

    template <Bool Value> inline constexpr Bool TBooleanConstantV = TBooleanConstant<Value>::Value;

} // namespace Internal

namespace Concepts
{

    /**
     * @concept CIsBool
     * @brief Checks if a type is Bool (cv-qualified variants included).
     */
    template <class T>
    concept CIsBool = Internal::TIsBoolV<T>;

} // namespace Concepts

} // namespace Lumen
