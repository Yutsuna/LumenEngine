/**
 * @file EnumFlags.hpp
 * @brief Defines macros for handling enum flags.
 */

#pragma once

#define LUMEN_ENUM_FLAGS( EnumType )                                                                                                                                     \
    [[nodiscard]] constexpr inline EnumType operator|( EnumType lhs, EnumType rhs )                                                                                      \
    {                                                                                                                                                                    \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                                                         \
        return static_cast<EnumType>( static_cast<UnderlyingType>( lhs ) | static_cast<UnderlyingType>( rhs ) );                                                         \
    }                                                                                                                                                                    \
    [[nodiscard]] constexpr inline EnumType operator&( EnumType lhs, EnumType rhs )                                                                                      \
    {                                                                                                                                                                    \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                                                         \
        return static_cast<EnumType>( static_cast<UnderlyingType>( lhs ) & static_cast<UnderlyingType>( rhs ) );                                                         \
    }                                                                                                                                                                    \
    [[nodiscard]] constexpr inline EnumType operator^( EnumType lhs, EnumType rhs )                                                                                      \
    {                                                                                                                                                                    \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                                                         \
        return static_cast<EnumType>( static_cast<UnderlyingType>( lhs ) ^ static_cast<UnderlyingType>( rhs ) );                                                         \
    }                                                                                                                                                                    \
    [[nodiscard]] constexpr inline EnumType operator~( EnumType value )                                                                                                  \
    {                                                                                                                                                                    \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                                                         \
        return static_cast<EnumType>( ~static_cast<UnderlyingType>( value ) );                                                                                           \
    }                                                                                                                                                                    \
    [[nodiscard]] constexpr inline EnumType &operator|=( EnumType &lhs, EnumType rhs )                                                                                   \
    {                                                                                                                                                                    \
        lhs = lhs | rhs;                                                                                                                                                 \
        return lhs;                                                                                                                                                      \
    }                                                                                                                                                                    \
    [[nodiscard]] constexpr inline EnumType &operator&=( EnumType &lhs, EnumType rhs )                                                                                   \
    {                                                                                                                                                                    \
        lhs = lhs & rhs;                                                                                                                                                 \
        return lhs;                                                                                                                                                      \
    }                                                                                                                                                                    \
    [[nodiscard]] constexpr inline EnumType &operator^=( EnumType &lhs, EnumType rhs )                                                                                   \
    {                                                                                                                                                                    \
        lhs = lhs ^ rhs;                                                                                                                                                 \
        return lhs;                                                                                                                                                      \
    }                                                                                                                                                                    \
    [[nodiscard]] constexpr inline bool operator!( EnumType value )                                                                                                      \
    {                                                                                                                                                                    \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                                                         \
        return static_cast<UnderlyingType>( value ) == 0;                                                                                                                \
    }                                                                                                                                                                    \
    [[nodiscard]] constexpr inline bool operator==( EnumType lhs, int rhs )                                                                                              \
    {                                                                                                                                                                    \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                                                         \
        return static_cast<UnderlyingType>( lhs ) == rhs;                                                                                                                \
    }                                                                                                                                                                    \
    [[nodiscard]] constexpr inline bool operator!=( EnumType lhs, int rhs )                                                                                              \
    {                                                                                                                                                                    \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                                                         \
        return static_cast<UnderlyingType>( lhs ) != rhs;                                                                                                                \
    }