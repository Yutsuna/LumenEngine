/**
 * @file LaunchLaunchModuleStackTests.cpp
 * @brief Functional tests for FLaunchModuleStack startup, teardown and dispatch.
 */

#include <Launch/LaunchModuleStack.hpp>

#include <Engine/LaunchEngineModule.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

/**
 * Public
 */

namespace
{

    UInt32 GFixedTickCount    = 0U;
    Float64 GLastFixedDelta   = 0.0;
    UInt32 GTickCount         = 0U;
    Float64 GLastDelta        = 0.0;
    Float64 GLastAlpha        = 0.0;
    Float64 GFactoryFixedStep = -1.0;

    class FFixedTickModule final : public Launch::IEngineModule
    {
    public:

        [[nodiscard]] FStringView GetName () const noexcept override
        {
            return "FixedTick";
        }

        [[nodiscard]] Launch::EModuleTraits GetTraits () const noexcept override
        {
            return Launch::EModuleTraits::FixedTick;
        }

        void FixedTick ( const Float64 InFixedDeltaSeconds ) noexcept override
        {
            GLastFixedDelta = InFixedDeltaSeconds;
            ++GFixedTickCount;
        }
    };

    class FTickModule final : public Launch::IEngineModule
    {
    public:

        [[nodiscard]] FStringView GetName () const noexcept override
        {
            return "Tick";
        }

        [[nodiscard]] Launch::EModuleTraits GetTraits () const noexcept override
        {
            return Launch::EModuleTraits::Tick;
        }

        void Tick ( const Float64 InDeltaSeconds, const Float64 InAlpha ) noexcept override
        {
            GLastDelta = InDeltaSeconds;
            GLastAlpha = InAlpha;
            ++GTickCount;
        }
    };

    class FHybridModule final : public Launch::IEngineModule
    {
    public:

        [[nodiscard]] FStringView GetName () const noexcept override
        {
            return "Hybrid";
        }

        [[nodiscard]] Launch::EModuleTraits GetTraits () const noexcept override
        {
            return Launch::EModuleTraits::FixedTick | Launch::EModuleTraits::Tick;
        }

        void FixedTick ( const Float64 InFixedDeltaSeconds ) noexcept override
        {
            GLastFixedDelta = InFixedDeltaSeconds;
            ++GFixedTickCount;
        }

        void Tick ( const Float64 InDeltaSeconds, const Float64 InAlpha ) noexcept override
        {
            GLastDelta = InDeltaSeconds;
            GLastAlpha = InAlpha;
            ++GTickCount;
        }
    };

    class FNoOpModule final : public Launch::IEngineModule
    {
    public:

        [[nodiscard]] FStringView GetName () const noexcept override
        {
            return "NoOp";
        }
    };

    class FLifetimeTrackerModule final : public Launch::IEngineModule
    {
    public:

        explicit FLifetimeTrackerModule ( const FStringView InName, TVector<FStringView> &InEvents ) noexcept
            : Name( InName ), Events( InEvents )
        {
            Events.push_back( "Start" );
            Events.push_back( Name );
        }

        ~FLifetimeTrackerModule () noexcept override
        {
            Events.push_back( "Stop" );
            Events.push_back( Name );
        }

        [[nodiscard]] FStringView GetName () const noexcept override
        {
            return Name;
        }

    private:

        FStringView Name;
        TVector<FStringView> &Events;
    };

    TVector<FStringView> GLifetimeEvents;

    [[nodiscard]] Launch::FModuleResult MakeFixedTickModule ( const Launch::FModuleContext & ) noexcept
    {
        return MakeUnique<FFixedTickModule>();
    }

    [[nodiscard]] Launch::FModuleResult MakeTickModule ( const Launch::FModuleContext & ) noexcept
    {
        return MakeUnique<FTickModule>();
    }

    [[nodiscard]] Launch::FModuleResult MakeNoOpModule ( const Launch::FModuleContext & ) noexcept
    {
        return MakeUnique<FNoOpModule>();
    }

    [[nodiscard]] Launch::FModuleResult MakeHybridModule ( const Launch::FModuleContext & ) noexcept
    {
        return MakeUnique<FHybridModule>();
    }

    /** @brief Records the FixedStep it received so tests can assert config forwarding. */
    [[nodiscard]] Launch::FModuleResult MakeConfigProbeModule ( const Launch::FModuleContext &InContext ) noexcept
    {
        GFactoryFixedStep = InContext.GetConfig().FixedStep;
        return MakeUnique<FNoOpModule>();
    }

    [[nodiscard]] Launch::FModuleResult MakeFailingModule ( const Launch::FModuleContext & ) noexcept
    {
        return LaunchFailure( Launch::ELaunchStage::Init, Launch::ELaunchErrorCode::ModuleCreationFailed, "boom" );
    }

    [[nodiscard]] Launch::FModuleResult MakeNullModule ( const Launch::FModuleContext & ) noexcept
    {
        return TUniquePtr<Launch::IEngineModule>{};
    }

    [[nodiscard]] Launch::FModuleResult MakeFirstTrackerModule ( const Launch::FModuleContext & ) noexcept
    {
        return MakeUnique<FLifetimeTrackerModule>( "First", GLifetimeEvents );
    }

    [[nodiscard]] Launch::FModuleResult MakeSecondTrackerModule ( const Launch::FModuleContext & ) noexcept
    {
        return MakeUnique<FLifetimeTrackerModule>( "Second", GLifetimeEvents );
    }

    [[nodiscard]] Launch::FModuleResult MakeThirdTrackerModule ( const Launch::FModuleContext & ) noexcept
    {
        return MakeUnique<FLifetimeTrackerModule>( "Third", GLifetimeEvents );
    }

    using FCreateModule = Launch::FModuleResult ( * )( const Launch::FModuleContext & ) noexcept;

    /** @brief Builds a standalone descriptor; LoadPriority makes the startup order explicit in tests. */
    [[nodiscard]] constexpr Launch::FModuleDescriptor
    MakeDescriptor ( const Launch::FModuleId InId, const FStringView InName, const FCreateModule InCreate,
                     const Int32 InLoadPriority = 0 ) noexcept
    {
        return Launch::FModuleDescriptor{
            .Id               = InId,
            .Name             = InName,
            .QualifiedName    = InName,
            .Provides         = {},
            .Requires         = {},
            .OptionalRequires = {},
            .Before           = {},
            .CliOptions       = {},
            .TickPhase        = Launch::ETickPhase::Update,
            .LoadPriority     = InLoadPriority,
            .IsEnabled        = nullptr,
            .Create           = InCreate,
        };
    }

} // namespace

LUMEN_TEST( FLaunchModuleStack_Construct, IsEmpty )
{
    const Launch::FLaunchModuleStack Stack;

    LUMEN_CHECK_EQ( Stack.Num(), 0ULL );
}

LUMEN_TEST( FLaunchModuleStack_StartAll, SucceedsWithValidDescriptors )
{
    Launch::FLaunchModuleStack Stack;

    static constexpr Launch::FModuleDescriptor Descriptor = MakeDescriptor( 1ULL, "NoOp", &MakeNoOpModule );
    static constexpr const Launch::FModuleDescriptor *Descriptors[] = { &Descriptor };

    const auto Result = Stack.StartAll( Descriptors, Launch::FLaunchEngineConfig{} );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_EQ( Stack.Num(), 1ULL );
}

LUMEN_TEST( FLaunchModuleStack_StartAll, PreservesStartupOrder )
{
    Launch::FLaunchModuleStack Stack;

    static constexpr Launch::FModuleDescriptor First  = MakeDescriptor( 1ULL, "First", &MakeNoOpModule );
    static constexpr Launch::FModuleDescriptor Second = MakeDescriptor( 2ULL, "Second", &MakeNoOpModule );
    static constexpr Launch::FModuleDescriptor Third  = MakeDescriptor( 3ULL, "Third", &MakeNoOpModule );
    static constexpr const Launch::FModuleDescriptor *Descriptors[] = { &First, &Second, &Third };

    const auto Result = Stack.StartAll( Descriptors, Launch::FLaunchEngineConfig{} );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_EQ( Stack.Num(), 3ULL );
}

LUMEN_TEST( FLaunchModuleStack_StartAll, FailsWhenFactoryReturnsError )
{
    Launch::FLaunchModuleStack Stack;

    static constexpr Launch::FModuleDescriptor NoOp    = MakeDescriptor( 1ULL, "NoOp", &MakeNoOpModule );
    static constexpr Launch::FModuleDescriptor Failing = MakeDescriptor( 2ULL, "Failing", &MakeFailingModule );
    static constexpr const Launch::FModuleDescriptor *Descriptors[] = { &NoOp, &Failing };

    const auto Result = Stack.StartAll( Descriptors, Launch::FLaunchEngineConfig{} );

    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK_EQ( Result.Error().Code, Launch::ELaunchErrorCode::ModuleCreationFailed );
    LUMEN_CHECK_EQ( Stack.Num(), 0ULL );
}

LUMEN_TEST( FLaunchModuleStack_StartAll, FailsWhenFactoryReturnsNull )
{
    Launch::FLaunchModuleStack Stack;

    static constexpr Launch::FModuleDescriptor Descriptor = MakeDescriptor( 1ULL, "Null", &MakeNullModule );
    static constexpr const Launch::FModuleDescriptor *Descriptors[] = { &Descriptor };

    const auto Result = Stack.StartAll( Descriptors, Launch::FLaunchEngineConfig{} );

    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK_EQ( Result.Error().Code, Launch::ELaunchErrorCode::ModuleCreationFailed );
    LUMEN_CHECK_EQ( Stack.Num(), 0ULL );
}

LUMEN_TEST( FLaunchModuleStack_StartAll, FailsWhenDescriptorHasNoFactory )
{
    Launch::FLaunchModuleStack Stack;

    static constexpr Launch::FModuleDescriptor Descriptor = MakeDescriptor( 1ULL, "Missing", nullptr );
    static constexpr const Launch::FModuleDescriptor *Descriptors[] = { &Descriptor };

    const auto Result = Stack.StartAll( Descriptors, Launch::FLaunchEngineConfig{} );

    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK_EQ( Result.Error().Code, Launch::ELaunchErrorCode::ModuleCreationFailed );
}

LUMEN_TEST( FLaunchModuleStack_FixedTick, DispatchesOnlyToFixedTickModules )
{
    GFixedTickCount = 0U;
    GTickCount      = 0U;

    Launch::FLaunchModuleStack Stack;

    static constexpr Launch::FModuleDescriptor FixedTick = MakeDescriptor( 1ULL, "FixedTick", &MakeFixedTickModule );
    static constexpr Launch::FModuleDescriptor Tick      = MakeDescriptor( 2ULL, "Tick", &MakeTickModule );
    static constexpr Launch::FModuleDescriptor Hybrid    = MakeDescriptor( 3ULL, "Hybrid", &MakeHybridModule );
    static constexpr const Launch::FModuleDescriptor *Descriptors[] = { &FixedTick, &Tick, &Hybrid };

    const auto Started = Stack.StartAll( Descriptors, Launch::FLaunchEngineConfig{} );
    LUMEN_CHECK_HAS_VALUE( Started );

    Stack.FixedTick( 0.016 );

    LUMEN_CHECK_EQ( GFixedTickCount, 2U );
    LUMEN_CHECK_EQ( GLastFixedDelta, 0.016 );
    LUMEN_CHECK_EQ( GTickCount, 0U );
}

LUMEN_TEST( FLaunchModuleStack_Tick, DispatchesOnlyToTickModules )
{
    GFixedTickCount = 0U;
    GTickCount      = 0U;

    Launch::FLaunchModuleStack Stack;

    static constexpr Launch::FModuleDescriptor FixedTick = MakeDescriptor( 1ULL, "FixedTick", &MakeFixedTickModule );
    static constexpr Launch::FModuleDescriptor Tick      = MakeDescriptor( 2ULL, "Tick", &MakeTickModule );
    static constexpr Launch::FModuleDescriptor Hybrid    = MakeDescriptor( 3ULL, "Hybrid", &MakeHybridModule );
    static constexpr const Launch::FModuleDescriptor *Descriptors[] = { &FixedTick, &Tick, &Hybrid };

    const auto Started = Stack.StartAll( Descriptors, Launch::FLaunchEngineConfig{} );
    LUMEN_CHECK_HAS_VALUE( Started );

    Stack.Tick( 0.016, 0.5 );

    LUMEN_CHECK_EQ( GTickCount, 2U );
    LUMEN_CHECK_EQ( GLastDelta, 0.016 );
    LUMEN_CHECK_EQ( GLastAlpha, 0.5 );
    LUMEN_CHECK_EQ( GFixedTickCount, 0U );
}

LUMEN_TEST( FLaunchModuleStack_StartAll, ForwardsConfigToFactories )
{
    GFactoryFixedStep = -1.0;

    Launch::FLaunchModuleStack Stack;

    Launch::FLaunchEngineConfig Config;
    Config.FixedStep = 0.125;

    static constexpr Launch::FModuleDescriptor Descriptor = MakeDescriptor( 1ULL, "Probe", &MakeConfigProbeModule );
    static constexpr const Launch::FModuleDescriptor *Descriptors[] = { &Descriptor };

    const auto Started = Stack.StartAll( Descriptors, Config );

    LUMEN_CHECK_HAS_VALUE( Started );
    LUMEN_CHECK_EQ( GFactoryFixedStep, 0.125 );
}

LUMEN_TEST( FLaunchModuleStack_StartAll, AcceptsEmptyDescriptorList )
{
    Launch::FLaunchModuleStack Stack;

    const auto Result = Stack.StartAll( {}, Launch::FLaunchEngineConfig{} );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_EQ( Stack.Num(), 0ULL );
}

LUMEN_TEST( FLaunchModuleStack_StartAll, ReportsInitStageOnFailure )
{
    Launch::FLaunchModuleStack Stack;

    static constexpr Launch::FModuleDescriptor Descriptor = MakeDescriptor( 1ULL, "Failing", &MakeFailingModule );
    static constexpr const Launch::FModuleDescriptor *Descriptors[] = { &Descriptor };

    const auto Result = Stack.StartAll( Descriptors, Launch::FLaunchEngineConfig{} );

    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK_EQ( Result.Error().Stage, Launch::ELaunchStage::Init );
}

LUMEN_TEST( FLaunchModuleStack_StartAll, RollsBackStartedModulesWhenALaterFactoryFails )
{
    GLifetimeEvents.clear();

    Launch::FLaunchModuleStack Stack;

    static constexpr Launch::FModuleDescriptor Tracker = MakeDescriptor( 1ULL, "First", &MakeFirstTrackerModule, 0 );
    static constexpr Launch::FModuleDescriptor Failing = MakeDescriptor( 2ULL, "Failing", &MakeFailingModule, 1 );
    static constexpr const Launch::FModuleDescriptor *Descriptors[] = { &Tracker, &Failing };

    const auto Result = Stack.StartAll( Descriptors, Launch::FLaunchEngineConfig{} );

    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK_EQ( Stack.Num(), 0ULL );

    /** INFO: A failed startup rolls every earlier module back through the destructor. */
    LUMEN_CHECK_EQ( GLifetimeEvents.size(), 4ULL );
    LUMEN_CHECK_TRUE( GLifetimeEvents[0] == "Start" );
    LUMEN_CHECK_TRUE( GLifetimeEvents[1] == "First" );
    LUMEN_CHECK_TRUE( GLifetimeEvents[2] == "Stop" );
    LUMEN_CHECK_TRUE( GLifetimeEvents[3] == "First" );
}

LUMEN_TEST( FLaunchModuleStack_ShutdownAll, TearsDownInReverseStartupOrder )
{
    GLifetimeEvents.clear();

    Launch::FLaunchModuleStack Stack;

    static constexpr Launch::FModuleDescriptor First  = MakeDescriptor( 1ULL, "First", &MakeFirstTrackerModule );
    static constexpr Launch::FModuleDescriptor Second = MakeDescriptor( 2ULL, "Second", &MakeSecondTrackerModule );
    static constexpr Launch::FModuleDescriptor Third  = MakeDescriptor( 3ULL, "Third", &MakeThirdTrackerModule );
    static constexpr const Launch::FModuleDescriptor *Descriptors[] = { &First, &Second, &Third };

    const auto Started = Stack.StartAll( Descriptors, Launch::FLaunchEngineConfig{} );
    LUMEN_CHECK_HAS_VALUE( Started );

    Stack.ShutdownAll();

    LUMEN_CHECK_EQ( GLifetimeEvents.size(), 12ULL );

    LUMEN_CHECK_TRUE( GLifetimeEvents[0] == "Start" );
    LUMEN_CHECK_TRUE( GLifetimeEvents[1] == "First" );
    LUMEN_CHECK_TRUE( GLifetimeEvents[2] == "Start" );
    LUMEN_CHECK_TRUE( GLifetimeEvents[3] == "Second" );
    LUMEN_CHECK_TRUE( GLifetimeEvents[4] == "Start" );
    LUMEN_CHECK_TRUE( GLifetimeEvents[5] == "Third" );

    LUMEN_CHECK_TRUE( GLifetimeEvents[6] == "Stop" );
    LUMEN_CHECK_TRUE( GLifetimeEvents[7] == "Third" );
    LUMEN_CHECK_TRUE( GLifetimeEvents[8] == "Stop" );
    LUMEN_CHECK_TRUE( GLifetimeEvents[9] == "Second" );
    LUMEN_CHECK_TRUE( GLifetimeEvents[10] == "Stop" );
    LUMEN_CHECK_TRUE( GLifetimeEvents[11] == "First" );
}

LUMEN_TEST( FLaunchModuleStack_ShutdownAll, IsIdempotent )
{
    GLifetimeEvents.clear();

    Launch::FLaunchModuleStack Stack;

    static constexpr Launch::FModuleDescriptor Descriptor = MakeDescriptor( 1ULL, "First", &MakeFirstTrackerModule );
    static constexpr const Launch::FModuleDescriptor *Descriptors[] = { &Descriptor };

    const auto Started = Stack.StartAll( Descriptors, Launch::FLaunchEngineConfig{} );
    LUMEN_CHECK_HAS_VALUE( Started );

    Stack.ShutdownAll();
    const USize EventsAfterFirstShutdown = GLifetimeEvents.size();

    Stack.ShutdownAll();

    LUMEN_CHECK_EQ( Stack.Num(), 0ULL );
    LUMEN_CHECK_EQ( GLifetimeEvents.size(), EventsAfterFirstShutdown );
}

LUMEN_TEST( FLaunchModuleStack_Move, TransfersOwnership )
{
    GLifetimeEvents.clear();

    Launch::FLaunchModuleStack Source;

    static constexpr Launch::FModuleDescriptor First  = MakeDescriptor( 1ULL, "First", &MakeFirstTrackerModule );
    static constexpr Launch::FModuleDescriptor Second = MakeDescriptor( 2ULL, "Second", &MakeSecondTrackerModule );
    static constexpr const Launch::FModuleDescriptor *Descriptors[] = { &First, &Second };

    const auto Started = Source.StartAll( Descriptors, Launch::FLaunchEngineConfig{} );
    LUMEN_CHECK_HAS_VALUE( Started );

    Launch::FLaunchModuleStack Destination( std::move( Source ) );

    LUMEN_CHECK_EQ( Source.Num(), 0ULL );
    LUMEN_CHECK_EQ( Destination.Num(), 2ULL );

    Destination.ShutdownAll();

    /** INFO: The moved-to stack owns the modules and tears them down in reverse order. */
    LUMEN_CHECK_TRUE( GLifetimeEvents[GLifetimeEvents.size() - 4] == "Stop" );
    LUMEN_CHECK_TRUE( GLifetimeEvents[GLifetimeEvents.size() - 3] == "Second" );
    LUMEN_CHECK_TRUE( GLifetimeEvents[GLifetimeEvents.size() - 2] == "Stop" );
    LUMEN_CHECK_TRUE( GLifetimeEvents[GLifetimeEvents.size() - 1] == "First" );
}

LUMEN_TEST( FLaunchModuleStack_Destructor, TearsDownModulesInReverseOrder )
{
    GLifetimeEvents.clear();

    {
        Launch::FLaunchModuleStack Stack;

        static constexpr Launch::FModuleDescriptor First  = MakeDescriptor( 1ULL, "First", &MakeFirstTrackerModule );
        static constexpr Launch::FModuleDescriptor Second = MakeDescriptor( 2ULL, "Second", &MakeSecondTrackerModule );
        static constexpr Launch::FModuleDescriptor Third  = MakeDescriptor( 3ULL, "Third", &MakeThirdTrackerModule );
        static constexpr const Launch::FModuleDescriptor *Descriptors[] = { &First, &Second, &Third };

        const auto Started = Stack.StartAll( Descriptors, Launch::FLaunchEngineConfig{} );
        LUMEN_CHECK_HAS_VALUE( Started );
    }

    LUMEN_CHECK_EQ( GLifetimeEvents.size(), 12ULL );

    LUMEN_CHECK_TRUE( GLifetimeEvents[6] == "Stop" );
    LUMEN_CHECK_TRUE( GLifetimeEvents[7] == "Third" );
    LUMEN_CHECK_TRUE( GLifetimeEvents[8] == "Stop" );
    LUMEN_CHECK_TRUE( GLifetimeEvents[9] == "Second" );
    LUMEN_CHECK_TRUE( GLifetimeEvents[10] == "Stop" );
    LUMEN_CHECK_TRUE( GLifetimeEvents[11] == "First" );
}

} // namespace Lumen
