/// @file UnitTestTypes.hpp
/// @brief Defines types and structures used for unit testing.

#pragma once

#include <Core/CoreTypes.hpp>
#include <CoreExport.hpp>

#include <String/StringView.hpp>

#include <source_location>

namespace Lumen
{

namespace Tests
{
    ///
    /// @enum EUnitTestKind
    /// @brief Qualifies the operational scope of a test case.
    ///
    enum class CORE_EXPORT EUnitTestKind : UInt8
    {
        Functional,
        Integration
    };

    ///
    /// @struct FUnitTestMetadata
    /// @brief Descriptor holding metadata for a unit test.
    ///
    struct CORE_EXPORT FUnitTestMetadata
    {
        FStringView SuiteName;
        FStringView TestName;
        EUnitTestKind Kind{ EUnitTestKind::Functional };
        void ( *ExecutionFunction )();
        std::source_location DeclaredLocation;
    };

    ///
    /// @struct FUnitTestRunSummary
    /// @brief Aggregate result of a test suite execution pass.
    ///
    struct CORE_EXPORT FUnitTestTestRunSummary
    {
        USize TotalExecuted{ 0 };
        USize PassedCount{ 0 };
        USize FailedCount{ 0 };
        Float64 DurationSeconds{ 0.0 };

        [[nodiscard]] constexpr Bool IsSuccessful () const noexcept
        {
            return FailedCount == 0;
        }
    };

} // namespace Tests

} // namespace Lumen