/// @file UnitTest.hpp
/// @brief Unit Test Registry class definition

#pragma once

#include <CoreExport.hpp>

#include <Templates/Optional.hpp>
#include <Templates/Vector.hpp>
#include <Tests/UnitTestTypes.hpp>
#include <Tests/UnitTestsAssert.hpp>

#include <format>
#include <source_location>

namespace Lumen
{

namespace Tests
{

    ///
    /// @class FUnitTestRegistry
    /// @brief Registry collecting and executing all statically declared tests.
    ///
    class CORE_EXPORT FUnitTestRegistry
    {
    public:

        /// @brief Get the singleton instance of the unit test registry.
        [[nodiscard]] static FUnitTestRegistry &GetInstance () noexcept;

        /// @brief Register a unit test with the registry.
        void Register ( FUnitTestMetadata const &Metadata );

        /// @brief Run the unit tests
        /// @param InSuiteFilter Wildcard or substring filter applied to SuiteName.
        /// @param InKindFilter Optional filter applied to test kind.
        /// @return Test run summary.
        [[nodiscard]] FUnitTestTestRunSummary Run ( FStringView InSuiteFilter             = {},
                                                    TOptional<EUnitTestKind> InKindFilter = {} ) const;

    public:

        /// @return The number of registered unit tests.
        [[nodiscard]] USize GetRegisteredCount () const noexcept;

    private:

        FUnitTestRegistry () noexcept  = default;
        ~FUnitTestRegistry () noexcept = default;

    private:

        TVector<FUnitTestMetadata> RegisteredTests;
    };

    ///
    /// @struct FAutoRegisterUnitTest
    /// @brief Helper registering a test case at dynamic startup.
    ///
    struct FAutoRegisterUnitTest
    {
        FAutoRegisterUnitTest ( const FStringView InSuite,
                                const FStringView InName,
                                const EUnitTestKind InKind,
                                void ( *InFn )(),
                                const std::source_location InLocation = std::source_location::current() ) noexcept
        {
            FUnitTestRegistry::GetInstance().Register( FUnitTestMetadata{
                .SuiteName         = InSuite,
                .TestName          = InName,
                .Kind              = InKind,
                .ExecutionFunction = InFn,
                .DeclaredLocation  = InLocation,
            } );
        }
    };

#define LUMEN_TEST( SuiteName, TestName )                                                                                        \
    static void Lumen_Test_##SuiteName##_##TestName();                                                                           \
    static inline ::Lumen::Tests::FAutoRegisterUnitTest Lumen_Register_##SuiteName##_##TestName(                                 \
        #SuiteName, #TestName, ::Lumen::Tests::EUnitTestKind::Functional, &Lumen_Test_##SuiteName##_##TestName );                \
    static void Lumen_Test_##SuiteName##_##TestName()

#define LUMEN_INTEGRATION_TEST( SuiteName, TestName )                                                                            \
    static void Lumen_IntegrationTest_##SuiteName##_##TestName();                                                                \
    static inline ::Lumen::Tests::FAutoRegisterUnitTest Lumen_RegisterIntegration_##SuiteName##_##TestName(                      \
        #SuiteName, #TestName, ::Lumen::Tests::EUnitTestKind::Integration, &Lumen_IntegrationTest_##SuiteName##_##TestName );    \
    static void Lumen_IntegrationTest_##SuiteName##_##TestName()

} // namespace Tests

} // namespace Lumen