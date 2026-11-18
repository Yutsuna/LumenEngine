/**
 * @file LaunchProcessGuardTests.cpp
 * @brief Functional tests for FLaunchProcessGuard RAII setup/teardown.
 */

#include <Engine/LaunchProcessGuard.hpp>

#include <Tests/UnitTest.hpp>

#include <new>

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

LUMEN_TEST( FLaunchProcessGuard_Construct, ReplacesTheNewHandlerWhileAlive )
{
    const std::new_handler InitialHandler = std::get_new_handler();

    {
        const Launch::FLaunchProcessGuard Guard;

        LUMEN_CHECK_TRUE( std::get_new_handler() != nullptr );
        LUMEN_CHECK_TRUE( std::get_new_handler() != InitialHandler );
    }

    LUMEN_CHECK_TRUE( std::get_new_handler() == InitialHandler );
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
