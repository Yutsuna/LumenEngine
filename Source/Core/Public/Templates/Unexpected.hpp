/// @file Unexpected.hpp
/// @brief Definition of TUnexpected class

#pragma once

#include <type_traits>

namespace Lumen
{

///
/// @struct FUnexpect
/// @brief Wrapper to encapsulate explicit error
///
struct FUnexpect
{
    constexpr explicit FUnexpect() = default;
};

inline constexpr FUnexpect Unexpect{};

///
/// @class TUnexpected
/// @brief Wrapper to encapsulate explicit error
///
template <class ErrorType> class TUnexpected
{

public:

    constexpr explicit TUnexpected(const ErrorType &InError) noexcept : Err(InError)
    {
        /// Empty
    }

    constexpr explicit TUnexpected(ErrorType &&InError) noexcept : Err(std::move(InError))
    {
        /// Empty
    }

    template <class... Args>
        requires std::is_constructible_v<ErrorType, Args...>
    constexpr explicit TUnexpected(std::in_place_t, Args &&... InArgs) noexcept : Err(std::forward<Args>(InArgs)...)
    {
        /// Empty
    }

    constexpr decltype( auto ) Error(this auto &&Self) noexcept
    {
        return std::forward<decltype( Self )>(Self).Err;
    }

    constexpr decltype( auto ) Value(this auto &&Self) noexcept
    {
        return std::forward<decltype( Self )>(Self).Err;
    }

private:

    ErrorType Err;
};

/// @brief Deduction guide for TUnexpected
template <class ErrorType> TUnexpected(ErrorType) noexcept -> TUnexpected<ErrorType>;

/// @brief Helper function to create a TUnexpected
template <class ErrorType>
[[nodiscard]] constexpr TUnexpected<std::decay_t<ErrorType>> MakeUnexpected(ErrorType &&InError) noexcept
{
    return TUnexpected<std::decay_t<ErrorType>>(std::forward<ErrorType>(InError));
}

} // namespace Lumen