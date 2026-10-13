/// @file UnitTestAssertionFailure.hpp
/// @brief Exception raised upon an unrecoverable assertion checkpoint failure.

#pragma once

#include <Core/CoreTypes.hpp>
#include <CoreExport.hpp>

#include <String/String.hpp>

#include <exception>
#include <source_location>

namespace Lumen
{

namespace Tests
{
    ///
    /// @class FUnitTestAssertionFailure
    /// @brief Exception raised upon an unrecoverable assertion checkpoint failure.
    ///
    class CORE_EXPORT FUnitTestAssertionFailure final : public std::exception
    {
    public:

        explicit FUnitTestAssertionFailure ( FString InMessage,
                                             const std::source_location &InLocation = std::source_location::current() ) noexcept;

    public:

        [[nodiscard]] const AnsiChar *what () const noexcept override;

        [[nodiscard]] constexpr const std::source_location &GetLocation () const noexcept
        {
            return Location;
        }

    private:

        FString Message;
        std::source_location Location;
    };

} // namespace Tests

} // namespace Lumen