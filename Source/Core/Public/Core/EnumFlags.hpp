///
/// @file EnumFlags.hpp
/// @brief Defines macros for handling enum flags.
///

#pragma once

#include "CoreTypes.hpp"

#define LUMEN_ENUM_FLAGS( EnumType )                                                                                             \
    [[nodiscard]] constexpr inline EnumType operator|( EnumType Lhs, EnumType Rhs ) noexcept                                     \
    {                                                                                                                            \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                 \
        return static_cast<EnumType>( static_cast<UnderlyingType>( Lhs ) | static_cast<UnderlyingType>( Rhs ) );                 \
    }                                                                                                                            \
    [[nodiscard]] constexpr inline EnumType operator&( EnumType Lhs, EnumType Rhs ) noexcept                                     \
    {                                                                                                                            \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                 \
        return static_cast<EnumType>( static_cast<UnderlyingType>( Lhs ) & static_cast<UnderlyingType>( Rhs ) );                 \
    }                                                                                                                            \
    [[nodiscard]] constexpr inline EnumType operator^( EnumType Lhs, EnumType Rhs ) noexcept                                     \
    {                                                                                                                            \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                 \
        return static_cast<EnumType>( static_cast<UnderlyingType>( Lhs ) ^ static_cast<UnderlyingType>( Rhs ) );                 \
    }                                                                                                                            \
    [[nodiscard]] constexpr inline EnumType operator~( EnumType value ) noexcept                                                 \
    {                                                                                                                            \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                 \
        return static_cast<EnumType>( ~static_cast<UnderlyingType>( value ) );                                                   \
    }                                                                                                                            \
    [[nodiscard]] constexpr inline EnumType operator<<( EnumType value, Int32 Shift ) noexcept                                   \
    {                                                                                                                            \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                 \
        return static_cast<EnumType>( static_cast<UnderlyingType>( value ) << static_cast<UnderlyingType>( Shift ) );            \
    }                                                                                                                            \
    [[nodiscard]] constexpr inline EnumType operator>>( EnumType value, Int32 Shift ) noexcept                                   \
    {                                                                                                                            \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                 \
        return static_cast<EnumType>( static_cast<UnderlyingType>( value ) >> static_cast<UnderlyingType>( Shift ) );            \
    }                                                                                                                            \
    constexpr inline EnumType &operator|=( EnumType &Lhs, EnumType Rhs ) noexcept                                                \
    {                                                                                                                            \
        Lhs = Lhs | Rhs;                                                                                                         \
        return Lhs;                                                                                                              \
    }                                                                                                                            \
    constexpr inline EnumType &operator&=( EnumType &Lhs, EnumType Rhs ) noexcept                                                \
    {                                                                                                                            \
        Lhs = Lhs & Rhs;                                                                                                         \
        return Lhs;                                                                                                              \
    }                                                                                                                            \
    constexpr inline EnumType &operator^=( EnumType &Lhs, EnumType Rhs ) noexcept                                                \
    {                                                                                                                            \
        Lhs = Lhs ^ Rhs;                                                                                                         \
        return Lhs;                                                                                                              \
    }                                                                                                                            \
    [[nodiscard]] constexpr inline Bool operator!( EnumType value ) noexcept                                                     \
    {                                                                                                                            \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                 \
        return static_cast<UnderlyingType>( value ) == 0;                                                                        \
    }                                                                                                                            \
    [[nodiscard]] constexpr inline Bool operator==( EnumType Lhs, Int32 Rhs ) noexcept                                           \
    {                                                                                                                            \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                 \
        return static_cast<UnderlyingType>( Lhs ) == static_cast<UnderlyingType>( Rhs );                                         \
    }                                                                                                                            \
    [[nodiscard]] constexpr inline Bool operator!=( EnumType Lhs, Int32 Rhs ) noexcept                                           \
    {                                                                                                                            \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                 \
        return static_cast<UnderlyingType>( Lhs ) != static_cast<UnderlyingType>( Rhs );                                         \
    }                                                                                                                            \
    [[nodiscard]] constexpr inline Bool HasAnyFlags( EnumType InFlags, EnumType InMask ) noexcept                                \
    {                                                                                                                            \
        return ( InFlags & InMask ) != 0;                                                                                        \
    }
