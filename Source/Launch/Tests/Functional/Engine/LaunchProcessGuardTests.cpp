/**
 * @file LaunchProcessGuardTests.cpp
 * @brief Functional tests for FLaunchProcessGuard RAII setup/teardown.
 */

#include <Engine/LaunchProcessGuard.hpp>

#include <Tests/UnitTest.hpp>

namespace Lumen
{

/**
 * Public
 */

LUMEN_TEST( FLaunchProcessGuard_Construct, DoesNotCrash )
{
    const Launch::FLaunchProcessGuard Guard;

    LUMEN_CHECK_TRUE( true );
}

LUMEN_TEST( FLaunchProcessGuard_Lifetime, NestedGuardsAreSafe )
{
    const Launch::FLaunchProcessGuard Outer;
    {
        const Launch::FLaunchProcessGuard Inner;
        LUMEN_CHECK_TRUE( true );
    }

    LUMEN_CHECK_TRUE( true );
}

} // namespace Lumen
