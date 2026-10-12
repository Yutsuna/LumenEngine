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
        template <typename NumberType>
            requires( std::is_arithmetic_v<NumberType> and not std::is_same_v<NumberType, Bool> )
        [[nodiscard]] TLaunchResult<NumberType> GetNumber ( const FStringView InKey ) const noexcept;

    private:

        FStringView ProgramName;
        TSpan<const AnsiChar *const> Arguments;
    };
} // namespace Launch
} // namespace Lumen

#include "Inline/CommandLine.inl"
