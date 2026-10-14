///
/// @file LaunchError.hpp
/// @brief Error type shared by every stage of the launch pipeline.
///

#pragma once

#include <Core/CoreTypes.hpp>
#include <String/StringView.hpp>
#include <Templates/Expected.hpp>

#include <type_traits>

namespace Lumen
{
namespace Launch
{
    ///
    /// @enum ELaunchStage
    /// @brief Pipeline stage that produced an error.
    ///
    enum class ELaunchStage : UInt8
    {
        PreInit,
        Init,
        Run,
    };

    ///
    /// @enum ELaunchErrorCode
    /// @brief Stable error codes. The numeric value doubles as the process exit code.
    ///
    enum class ELaunchErrorCode : UInt8
    {
        None = 0,
        MissingArgument,
        InvalidArgument,
        InvalidConfiguration,
        ModuleCreationFailed,
        OutOfMemory,
        Unknown,
    };

    ///
    /// @struct FLaunchError
    /// @brief Trivially copyable error payload: no heap, no ownership.
    /// @warning `Message` must point to storage with static duration (string literal).
    ///
    struct FLaunchError
    {
        ELaunchStage Stage;
        ELaunchErrorCode Code;
        FStringView Message;

        [[nodiscard]] constexpr Int32 ToExitCode () const noexcept
        {
            return static_cast<Int32>( Code );
        }
    };

    static_assert( std::is_trivially_copyable_v<FLaunchError>, "FLaunchError travels by value through TExpected." );

    /// @brief Result type of every launch stage.
    template <typename Type> using TLaunchResult = TExpected<Type, FLaunchError>;

    /// @brief Builds the failure side of a TLaunchResult.
    [[nodiscard]] constexpr TUnexpected<FLaunchError>
    LaunchFailure ( const ELaunchStage InStage, const ELaunchErrorCode InCode, const FStringView InMessage ) noexcept
    {
        return TUnexpected<FLaunchError>( { .Stage = InStage, .Code = InCode, .Message = InMessage } );
    }

    /// @brief Converts a launch stage to a string.
    /// @param InStage The launch stage to convert.
    [[nodiscard]] constexpr const AnsiChar *ToCString ( const ELaunchStage InStage ) noexcept
    {
        switch ( InStage )
        {
        case ELaunchStage::PreInit:
            return "PreInit";
        case ELaunchStage::Init:
            return "Init";
        case ELaunchStage::Run:
            return "Run";
        }

        return "Unknown";
    }
} // namespace Launch
} // namespace Lumen
