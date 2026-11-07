/*
 * @file WorldSpatialRegistryProbeTests.cpp
 * @brief Diagnostic probes isolating the FSpatialRegistry publish contract.

Every probe observes the registry through the read snapshot only, and asserts
relative to its own baseline so leftover state from other suites does not shift
expectations. Each step is checked independently so the first diverging public
API call is pinpointed.
 */

#include <World/WorldSpatialRegistry.hpp>

#include <Tests/UnitTest.hpp>

namespace Lumen
{

/**
 * Test helpers
 */

namespace
{

    /**
    * @brief Self-cleaning registry session: unregisters every tracked id and
            flushes the snapshot both on demand and at scope exit, even when an
            assertion throws mid-test.
     */
    struct FProbeSession
    {
        FSpatialRegistry &Registry = FSpatialRegistry::Get();
        TVector<ActorID> TrackedIds;

        ~FProbeSession ()
        {
            for ( const ActorID EntityId : TrackedIds )
            {
                Registry.UnregisterSpatialEntity( EntityId );
            }

            if ( not TrackedIds.empty() )
            {
                Registry.Publish();
                Registry.SwapReadBuffers();
            }
        }

        void Track ( const ActorID InId )
        {
            Registry.RegisterSpatialEntity( InId );
            TrackedIds.push_back( InId );
        }

        [[nodiscard]] USize SnapshotSize () const noexcept
        {
            return Registry.GetReadSnapshot().EntityIDs.size();
        }
    };

} // namespace

/**
 * Probe 1: single register / unregister cycle propagates through publish+swap.
 */

LUMEN_TEST( FSpatialRegistry_Probe, RegisterUnregisterCycle )
{
    FProbeSession Session;

    Session.Registry.Publish();
    Session.Registry.SwapReadBuffers();

    const USize BaselineSize = Session.SnapshotSize();

    Session.Track( 8801ULL );
    Session.Registry.Publish();
    Session.Registry.SwapReadBuffers();

    LUMEN_CHECK_EQ( Session.SnapshotSize() - BaselineSize, 1ULL );

    Session.Registry.UnregisterSpatialEntity( 8801ULL );
    Session.Registry.Publish();
    Session.Registry.SwapReadBuffers();

    LUMEN_CHECK_EQ( Session.SnapshotSize() - BaselineSize, 0ULL );
}

/**
 * Probe 2: duplicate registration stays collapsed.
 */

LUMEN_TEST( FSpatialRegistry_Probe, DuplicateRegistrationCollapses )
{
    FProbeSession Session;

    Session.Registry.Publish();
    Session.Registry.SwapReadBuffers();

    const USize BaselineSize = Session.SnapshotSize();

    /** INFO: The first registration is tracked; the duplicate goes through the plain API. */
    Session.Track( 8811ULL );
    Session.Registry.RegisterSpatialEntity( 8811ULL );
    Session.Registry.Publish();
    Session.Registry.SwapReadBuffers();

    LUMEN_CHECK_EQ( Session.SnapshotSize() - BaselineSize, 1ULL );
}

/**
 * Probe 3: many publishes between two swaps still expose the freshest state.
 */

LUMEN_TEST( FSpatialRegistry_Probe, MultiplePublishesSingleSwap )
{
    FProbeSession Session;

    Session.Registry.Publish();
    Session.Registry.SwapReadBuffers();

    const USize BaselineSize = Session.SnapshotSize();

    Session.Track( 8821ULL );
    Session.Track( 8822ULL );

    Session.Registry.Publish();
    Session.Registry.Publish();
    Session.Registry.Publish();
    Session.Registry.SwapReadBuffers();

    LUMEN_CHECK_EQ( Session.SnapshotSize() - BaselineSize, 2ULL );

    Session.Registry.SwapReadBuffers();

    LUMEN_CHECK_EQ( Session.SnapshotSize() - BaselineSize, 2ULL );
}

/**
 * Probe 4: delta-only publishes never resurrect structural changes.
 */

LUMEN_TEST( FSpatialRegistry_Probe, DeltaPublishesBetweenStructuralChanges )
{
    FProbeSession Session;

    Session.Registry.Publish();
    Session.Registry.SwapReadBuffers();

    const USize BaselineSize = Session.SnapshotSize();

    Session.Track( 8831ULL );

    Session.Registry.Publish();
    Session.Registry.SwapReadBuffers();

    LUMEN_CHECK_EQ( Session.SnapshotSize() - BaselineSize, 1ULL );

    /** INFO: Warm up every buffer slot with extra publish+swap cycles. */
    Session.Registry.Publish();
    Session.Registry.SwapReadBuffers();
    Session.Registry.Publish();
    Session.Registry.SwapReadBuffers();

    LUMEN_CHECK_EQ( Session.SnapshotSize() - BaselineSize, 1ULL );
}

} // namespace Lumen
