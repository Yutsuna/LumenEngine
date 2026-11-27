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

    [[nodiscard]] Launch::TLaunchResult<TUniquePtr<Launch::IEngineModule>>
    MakeFixedTickModule ( const Launch::FLaunchEngineConfig & ) noexcept
    {
        return MakeUnique<FFixedTickModule>();
    }

    [[nodiscard]] Launch::TLaunchResult<TUniquePtr<Launch::IEngineModule>>
    MakeTickModule ( const Launch::FLaunchEngineConfig & ) noexcept
    {
        return MakeUnique<FTickModule>();
    }

    [[nodiscard]] Launch::TLaunchResult<TUniquePtr<Launch::IEngineModule>>
    MakeNoOpModule ( const Launch::FLaunchEngineConfig & ) noexcept
    {
        return MakeUnique<FNoOpModule>();
    }

    [[nodiscard]] Launch::TLaunchResult<TUniquePtr<Launch::IEngineModule>>
    MakeHybridModule ( const Launch::FLaunchEngineConfig & ) noexcept
    {
        return MakeUnique<FHybridModule>();
    }

    /** @brief Records the FixedStep it received so tests can assert config forwarding. */
    [[nodiscard]] Launch::TLaunchResult<TUniquePtr<Launch::IEngineModule>>
    MakeConfigProbeModule ( const Launch::FLaunchEngineConfig &InConfig ) noexcept
    {
        GFactoryFixedStep = InConfig.FixedStep;
        return MakeUnique<FNoOpModule>();
    }

    [[nodiscard]] Launch::TLaunchResult<TUniquePtr<Launch::IEngineModule>>
    MakeFailingModule ( const Launch::FLaunchEngineConfig & ) noexcept
    {
        return Launch::LaunchFailure( Launch::ELaunchStage::Init, Launch::ELaunchErrorCode::ModuleCreationFailed, "boom" );
    }

    [[nodiscard]] Launch::TLaunchResult<TUniquePtr<Launch::IEngineModule>>
    MakeNullModule ( const Launch::FLaunchEngineConfig & ) noexcept
    {
        return TUniquePtr<Launch::IEngineModule>{};
    }

    [[nodiscard]] Launch::TLaunchResult<TUniquePtr<Launch::IEngineModule>>
    MakeFirstTrackerModule ( const Launch::FLaunchEngineConfig & ) noexcept
    {
        return MakeUnique<FLifetimeTrackerModule>( "First", GLifetimeEvents );
    }

    [[nodiscard]] Launch::TLaunchResult<TUniquePtr<Launch::IEngineModule>>
    MakeSecondTrackerModule ( const Launch::FLaunchEngineConfig & ) noexcept
    {
        return MakeUnique<FLifetimeTrackerModule>( "Second", GLifetimeEvents );
    }

    [[nodiscard]] Launch::TLaunchResult<TUniquePtr<Launch::IEngineModule>>
    MakeThirdTrackerModule ( const Launch::FLaunchEngineConfig & ) noexcept
    {
        return MakeUnique<FLifetimeTrackerModule>( "Third", GLifetimeEvents );
    }

} // namespace

LUMEN_TEST( FLaunchModuleStack_Construct, IsEmpty )
{
    const Launch::FLaunchModuleStack Stack;

    LUMEN_CHECK_EQ( Stack.Num(), 0ULL );
}

LUMEN_TEST( FLaunchModuleStack_StartAll, SucceedsWithValidFactories )
{
    Launch::FLaunchModuleStack Stack;

    static constexpr Launch::FModuleFactory Factories[] = {
        &MakeNoOpModule,
    };

    const auto Result = Stack.StartAll( Factories, Launch::FLaunchEngineConfig{} );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_EQ( Stack.Num(), 1ULL );
}

LUMEN_TEST( FLaunchModuleStack_StartAll, PreservesStartupOrder )
{
    Launch::FLaunchModuleStack Stack;

    static constexpr Launch::FModuleFactory Factories[] = {
        &MakeNoOpModule,
        &MakeNoOpModule,
        &MakeNoOpModule,
    };

    const auto Result = Stack.StartAll( Factories, Launch::FLaunchEngineConfig{} );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_EQ( Stack.Num(), 3ULL );
}

LUMEN_TEST( FLaunchModuleStack_StartAll, FailsWhenFactoryReturnsError )
{
    Launch::FLaunchModuleStack Stack;

    static constexpr Launch::FModuleFactory Factories[] = {
        &MakeNoOpModule,
        &MakeFailingModule,
    };

    const auto Result = Stack.StartAll( Factories, Launch::FLaunchEngineConfig{} );

    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK_EQ( Result.Error().Code, Launch::ELaunchErrorCode::ModuleCreationFailed );
    LUMEN_CHECK_EQ( Stack.Num(), 1ULL );
}

LUMEN_TEST( FLaunchModuleStack_StartAll, FailsWhenFactoryReturnsNull )
{
    Launch::FLaunchModuleStack Stack;

    static constexpr Launch::FModuleFactory Factories[] = {
        &MakeNullModule,
    };

    const auto Result = Stack.StartAll( Factories, Launch::FLaunchEngineConfig{} );

    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK_EQ( Result.Error().Code, Launch::ELaunchErrorCode::ModuleCreationFailed );
    LUMEN_CHECK_EQ( Stack.Num(), 0ULL );
}

LUMEN_TEST( FLaunchModuleStack_StartAll, FailsWhenDescriptorHasNoFactory )
{
    Launch::FLaunchModuleStack Stack;

    static constexpr Launch::FModuleFactory Factories[] = {
        nullptr,
    };

    const auto Result = Stack.StartAll( Factories, Launch::FLaunchEngineConfig{} );

    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK_EQ( Result.Error().Code, Launch::ELaunchErrorCode::ModuleCreationFailed );
}

LUMEN_TEST( FLaunchModuleStack_FixedTick, DispatchesOnlyToFixedTickModules )
{
    GFixedTickCount = 0U;
    GTickCount      = 0U;

    Launch::FLaunchModuleStack Stack;

    static constexpr Launch::FModuleFactory Factories[] = {
        &MakeFixedTickModule,
        &MakeTickModule,
        &MakeHybridModule,
    };

    const auto Started = Stack.StartAll( Factories, Launch::FLaunchEngineConfig{} );
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

    static constexpr Launch::FModuleFactory Factories[] = {
        &MakeFixedTickModule,
        &MakeTickModule,
        &MakeHybridModule,
    };

    const auto Started = Stack.StartAll( Factories, Launch::FLaunchEngineConfig{} );
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

    static constexpr Launch::FModuleFactory Factories[] = {
        &MakeConfigProbeModule,
    };

    const auto Started = Stack.StartAll( Factories, Config );

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

    static constexpr Launch::FModuleFactory Factories[] = {
        &MakeFailingModule,
    };

    const auto Result = Stack.StartAll( Factories, Launch::FLaunchEngineConfig{} );

    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK_EQ( Result.Error().Stage, Launch::ELaunchStage::Init );
}

LUMEN_TEST( FLaunchModuleStack_StartAll, KeepsStartedModulesWhenALaterFactoryFails )
{
    GLifetimeEvents.clear();

    Launch::FLaunchModuleStack Stack;

    static constexpr Launch::FModuleFactory Factories[] = {
        &MakeFirstTrackerModule,
        &MakeFailingModule,
    };

    const auto Result = Stack.StartAll( Factories, Launch::FLaunchEngineConfig{} );

    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK_EQ( Stack.Num(), 1ULL );

    const USize EventsBeforeTeardown = GLifetimeEvents.size();

    Stack.ShutdownAll();

    /** INFO: The kept module must be torn down exactly once, by the explicit shutdown. */
    LUMEN_CHECK_TRUE( GLifetimeEvents.size() > EventsBeforeTeardown );
    LUMEN_CHECK_TRUE( GLifetimeEvents[EventsBeforeTeardown] == "Stop" );
    LUMEN_CHECK_TRUE( GLifetimeEvents[EventsBeforeTeardown + 1] == "First" );
    LUMEN_CHECK_EQ( GLifetimeEvents.size(), EventsBeforeTeardown + 2ULL );
}

LUMEN_TEST( FLaunchModuleStack_ShutdownAll, TearsDownInReverseStartupOrder )
{
    GLifetimeEvents.clear();

    Launch::FLaunchModuleStack Stack;

    static constexpr Launch::FModuleFactory Factories[] = {
        &MakeFirstTrackerModule,
        &MakeSecondTrackerModule,
        &MakeThirdTrackerModule,
    };

    const auto Started = Stack.StartAll( Factories, Launch::FLaunchEngineConfig{} );
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

    static constexpr Launch::FModuleFactory Factories[] = {
        &MakeFirstTrackerModule,
    };

    const auto Started = Stack.StartAll( Factories, Launch::FLaunchEngineConfig{} );
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

    static constexpr Launch::FModuleFactory Factories[] = {
        &MakeFirstTrackerModule,
        &MakeSecondTrackerModule,
    };

    const auto Started = Source.StartAll( Factories, Launch::FLaunchEngineConfig{} );
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

        static constexpr Launch::FModuleFactory Factories[] = {
            &MakeFirstTrackerModule,
            &MakeSecondTrackerModule,
            &MakeThirdTrackerModule,
        };

        const auto Started = Stack.StartAll( Factories, Launch::FLaunchEngineConfig{} );
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
