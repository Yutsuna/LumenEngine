/**
 * @file EnumFlags.hpp
 * @brief Defines macros for handling enum flags.
 */

#pragma once

#include "CoreTypes.hpp"

#define LUMEN_ENUM_FLAGS( EnumType )                                                                                                                                     \
    /** Binary operators for enum flags */                                                                                                                               \
    [[nodiscard]] static constexpr inline EnumType operator|( EnumType Lhs, EnumType Rhs ) noexcept                                                                      \
    {                                                                                                                                                                    \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                                                         \
        return static_cast<EnumType>( static_cast<UnderlyingType>( Lhs ) | static_cast<UnderlyingType>( Rhs ) );                                                         \
    }                                                                                                                                                                    \
    [[nodiscard]] static constexpr inline EnumType operator&( EnumType Lhs, EnumType Rhs ) noexcept                                                                      \
    {                                                                                                                                                                    \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                                                         \
        return static_cast<EnumType>( static_cast<UnderlyingType>( Lhs ) & static_cast<UnderlyingType>( Rhs ) );                                                         \
    }                                                                                                                                                                    \
    [[nodiscard]] static constexpr inline EnumType operator^( EnumType Lhs, EnumType Rhs ) noexcept                                                                      \
    {                                                                                                                                                                    \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                                                         \
        return static_cast<EnumType>( static_cast<UnderlyingType>( Lhs ) ^ static_cast<UnderlyingType>( Rhs ) );                                                         \
    }                                                                                                                                                                    \
    [[nodiscard]] static constexpr inline EnumType operator~( EnumType value ) noexcept                                                                                  \
    {                                                                                                                                                                    \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                                                         \
        return static_cast<EnumType>( ~static_cast<UnderlyingType>( value ) );                                                                                           \
    }                                                                                                                                                                    \
    [[nodiscard]] static constexpr inline EnumType operator<<( EnumType value, Int32 Shift ) noexcept                                                                    \
    {                                                                                                                                                                    \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                                                         \
        return static_cast<EnumType>( static_cast<UnderlyingType>( value ) << static_cast<UnderlyingType>( Shift ) );                                                    \
    }                                                                                                                                                                    \
    [[nodiscard]] static constexpr inline EnumType operator>>( EnumType value, Int32 Shift ) noexcept                                                                    \
    {                                                                                                                                                                    \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                                                         \
        return static_cast<EnumType>( static_cast<UnderlyingType>( value ) >> static_cast<UnderlyingType>( Shift ) );                                                    \
    }                                                                                                                                                                    \
    /** Assignment operators for enum flags */                                                                                                                           \
    static constexpr inline EnumType &operator|=( EnumType &Lhs, EnumType Rhs ) noexcept                                                                                 \
    {                                                                                                                                                                    \
        Lhs = Lhs | Rhs;                                                                                                                                                 \
        return Lhs;                                                                                                                                                      \
    }                                                                                                                                                                    \
    static constexpr inline EnumType &operator&=( EnumType &Lhs, EnumType Rhs ) noexcept                                                                                 \
    {                                                                                                                                                                    \
        Lhs = Lhs & Rhs;                                                                                                                                                 \
        return Lhs;                                                                                                                                                      \
    }                                                                                                                                                                    \
    static constexpr inline EnumType &operator^=( EnumType &Lhs, EnumType Rhs ) noexcept                                                                                 \
    {                                                                                                                                                                    \
        Lhs = Lhs ^ Rhs;                                                                                                                                                 \
        return Lhs;                                                                                                                                                      \
    }                                                                                                                                                                    \
    /** Logical comparison operators for enum flags */                                                                                                                   \
    [[nodiscard]] static constexpr inline LumenEngine::Bool operator!( EnumType value ) noexcept                                                                         \
    {                                                                                                                                                                    \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                                                         \
        return static_cast<UnderlyingType>( value ) == 0;                                                                                                                \
    }                                                                                                                                                                    \
    [[nodiscard]] static constexpr inline LumenEngine::Bool operator==( EnumType Lhs, Int32 Rhs ) noexcept                                                               \
    {                                                                                                                                                                    \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                                                         \
        return static_cast<UnderlyingType>( Lhs ) == static_cast<UnderlyingType>( Rhs );                                                                                 \
    }                                                                                                                                                                    \
    [[nodiscard]] static constexpr inline LumenEngine::Bool operator!=( EnumType Lhs, Int32 Rhs ) noexcept                                                               \
    {                                                                                                                                                                    \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                                                         \
        return static_cast<UnderlyingType>( Lhs ) != static_cast<UnderlyingType>( Rhs );                                                                                 \
    }
