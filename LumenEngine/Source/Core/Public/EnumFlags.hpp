/**
 * @file EnumFlags.hpp
 * @brief Defines macros for handling enum flags.
 */

#pragma once

#define LUMEN_ENUM_FLAGS( EnumType )                                                                                                                                     \
    /** Binary operators for enum flags */                                                                                                                               \
    [[nodiscard]] constexpr inline EnumType operator|( EnumType lhs, EnumType rhs ) noexcept                                                                            \
    {                                                                                                                                                                    \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                                                         \
        return static_cast<EnumType>( static_cast<UnderlyingType>( lhs ) | static_cast<UnderlyingType>( rhs ) );                                                         \
    }                                                                                                                                                                    \
    [[nodiscard]] constexpr inline EnumType operator&( EnumType lhs, EnumType rhs ) noexcept                                                                            \
    {                                                                                                                                                                    \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                                                         \
        return static_cast<EnumType>( static_cast<UnderlyingType>( lhs ) & static_cast<UnderlyingType>( rhs ) );                                                         \
    }                                                                                                                                                                    \
    [[nodiscard]] constexpr inline EnumType operator^( EnumType lhs, EnumType rhs ) noexcept                                                                            \
    {                                                                                                                                                                    \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                                                         \
        return static_cast<EnumType>( static_cast<UnderlyingType>( lhs ) ^ static_cast<UnderlyingType>( rhs ) );                                                         \
    }                                                                                                                                                                    \
    [[nodiscard]] constexpr inline EnumType operator~( EnumType value ) noexcept                                                                                         \
    {                                                                                                                                                                    \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                                                         \
        return static_cast<EnumType>( ~static_cast<UnderlyingType>( value ) );                                                                                           \
    }                                                                                                                                                                    \
    /** Assignment operators for enum flags */                                                                                                                           \
    constexpr inline EnumType &operator|=( EnumType &lhs, EnumType rhs ) noexcept                                                                                        \
    {                                                                                                                                                                    \
        lhs = lhs | rhs;                                                                                                                                                 \
        return lhs;                                                                                                                                                      \
    }                                                                                                                                                                    \
    constexpr inline EnumType &operator&=( EnumType &lhs, EnumType rhs ) noexcept                                                                                        \
    {                                                                                                                                                                    \
        lhs = lhs & rhs;                                                                                                                                                 \
        return lhs;                                                                                                                                                      \
    }                                                                                                                                                                    \
    constexpr inline EnumType &operator^=( EnumType &lhs, EnumType rhs ) noexcept                                                                                        \
    {                                                                                                                                                                    \
        lhs = lhs ^ rhs;                                                                                                                                                 \
        return lhs;                                                                                                                                                      \
    }                                                                                                                                                                    \
    /** Logical comparison operators for enum flags */                                                                                                                   \
    [[nodiscard]] constexpr inline bool operator!( EnumType value ) noexcept                                                                                             \
    {                                                                                                                                                                    \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                                                         \
        return static_cast<UnderlyingType>( value ) == 0;                                                                                                                \
    }                                                                                                                                                                    \
    [[nodiscard]] constexpr inline bool operator==( EnumType lhs, int rhs ) noexcept                                                                                     \
    {                                                                                                                                                                    \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                                                         \
        return static_cast<UnderlyingType>( lhs ) == rhs;                                                                                                                \
    }                                                                                                                                                                    \
    [[nodiscard]] constexpr inline bool operator!=( EnumType lhs, int rhs ) noexcept                                                                                     \
    {                                                                                                                                                                    \
        using UnderlyingType = std::underlying_type_t<EnumType>;                                                                                                         \
        return static_cast<UnderlyingType>( lhs ) != rhs;                                                                                                                \
    }
