///
/// @file CommandLine.hpp
/// @brief Zero-allocation, non-owning view over `argv`.
///

#pragma once

#include <Core/CoreTypes.hpp>

#include <Launch/LaunchError.hpp>
#include <Memory/Span.hpp>
#include <String/StringView.hpp>

#include <type_traits>

namespace Lumen
{

namespace Launch
{
    namespace Concepts
    {
        /// @concept CCommandLineNumber
        /// @brief Numeric types FCommandLine can parse.
        template <typename Type>
        concept CCommandLineNumber = std::is_arithmetic_v<Type> and not std::is_same_v<Type, Bool>;
    } // namespace Concepts

    ///
    /// @class FCommandLine
    /// @brief Parses `-Switch`, `-Key=Value` and `--Key=Value` (keys are ASCII case-insensitive).
    ///
    class FCommandLine
    {
    public:

        FCommandLine () noexcept = default;
        FCommandLine ( Int32 InArgc, const AnsiChar *const *InArgv ) noexcept;

    public:

        [[nodiscard]] FStringView GetProgramName () const noexcept;

        /// @brief True if `-Name` is present (without a value).
        [[nodiscard]] Bool HasSwitch ( FStringView InName ) const noexcept;

        /// @brief Raw text of `-Key=Value`, or MissingArgument.
        [[nodiscard]] TLaunchResult<FStringView> GetValue ( FStringView InKey ) const noexcept;

        /// @brief Numeric value of `-Key=Value`; MissingArgument if absent, InvalidArgument if malformed.
        template <Concepts::CCommandLineNumber NumberType>
        [[nodiscard]] TLaunchResult<NumberType> GetNumber ( FStringView InKey ) const noexcept;

        /// @brief Like GetNumber, but an absent key yields `InFallback`. A malformed value is still an error.
        template <Concepts::CCommandLineNumber NumberType>
        [[nodiscard]] TLaunchResult<NumberType> GetNumberOr ( FStringView InKey, NumberType InFallback ) const noexcept;

    private:

        FStringView ProgramName;
        TSpan<const AnsiChar *const> Arguments;
    };
} // namespace Launch

} // namespace Lumen

#include "Inline/CommandLine.inl"
