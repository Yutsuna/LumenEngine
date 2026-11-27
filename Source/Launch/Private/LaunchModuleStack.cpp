/**
 * @file LaunchModuleStack.cpp
 * @brief FLaunchModuleStack implementation with deterministic topological DAG resolution.
 */

#include <Launch/LaunchModuleStack.hpp>

#include <Algo/ForEach.hpp>
#include <Logging/Logger.hpp>

#include <algorithm>
#include <limits>
#include <utility>

/**
 * Private
 */

namespace
{

LUMEN_LOG_DEFINE_CATEGORY( LogLaunchModuleStack, "LaunchModuleStack" )

constexpr Lumen::USize InvalidModuleIndex = std::numeric_limits<Lumen::USize>::max();

struct FIndexedProvider
{
    Lumen::Launch::FModuleId Id = 0;
    Lumen::FStringView Name;
    Lumen::FStringView QualifiedName;
    Lumen::USize OwnerIndex = 0;
    Lumen::FStringView OwnerName;
};

struct FTickEntry
{
    Lumen::Launch::IEngineModule *Module = nullptr;
    Lumen::Launch::ETickPhase Phase      = Lumen::Launch::ETickPhase::Update;
    Lumen::USize TopoIndex               = 0;
};

[[nodiscard]] const FIndexedProvider *FindProvider ( const Lumen::TVector<FIndexedProvider> &InProviders,
                                                     const Lumen::Launch::FModuleId InId ) noexcept
{
    for ( const FIndexedProvider &Entry : InProviders )
    {
        if ( Entry.Id == InId )
        {
            return &Entry;
        }
    }
    return nullptr;
}

[[nodiscard]] Lumen::Bool AddUniqueEdge ( Lumen::TVector<Lumen::TVector<Lumen::USize>> &InOutEdges,
                                          Lumen::TVector<Lumen::USize> &InOutInDegree,
                                          const Lumen::USize InFrom,
                                          const Lumen::USize InTo ) noexcept
{
    if ( InFrom == InTo ) [[unlikely]]
    {
        return false;
    }

    Lumen::TVector<Lumen::USize> &Targets = InOutEdges[InFrom];
    for ( const Lumen::USize Existing : Targets )
    {
        if ( Existing == InTo )
        {
            return true;
        }
    }

    Targets.push_back( InTo );
    ++InOutInDegree[InTo];
    return true;
}

} // namespace

/**
 * Public
 */

Lumen::Launch::FLaunchModuleStack::FLaunchModuleStack ( FLaunchModuleStack &&Other ) noexcept
    : Modules( std::move( Other.Modules ) ), Services( std::move( Other.Services ) ),
      FixedTickList( std::move( Other.FixedTickList ) ), TickList( std::move( Other.TickList ) )
{
    Other.Modules.clear();
    Other.Services.clear();
    Other.FixedTickList.clear();
    Other.TickList.clear();
}

Lumen::Launch::FLaunchModuleStack::~FLaunchModuleStack () noexcept
{
    ShutdownAll();
}

Lumen::Launch::TLaunchResult<void> Lumen::Launch::FLaunchModuleStack::StartAll ( const FLaunchEngineConfig &InConfig,
                                                                                 const FCommandLine &InCommandLine ) noexcept
{
    const TVector<const FModuleDescriptor *> Registered = CollectRegisteredModules();
    return StartAll( Registered, InConfig, InCommandLine );
}

Lumen::Launch::TLaunchResult<void>
Lumen::Launch::FLaunchModuleStack::StartAll ( const TSpan<const FModuleDescriptor *const> InDescriptors,
                                              const FLaunchEngineConfig &InConfig,
                                              const FCommandLine &InCommandLine ) noexcept
{
    ShutdownAll();

    TVector<const FModuleDescriptor *> Enabled;
    TVector<FIndexedProvider> ActiveProviders;
    TVector<FIndexedProvider> DisabledProviders;

    Enabled.reserve( InDescriptors.size() );

    /** 1. Partition enabled / disabled modules and verify factory pointers. */
    for ( const FModuleDescriptor *const Desc : InDescriptors )
    {
        if ( Desc == nullptr ) [[unlikely]]
        {
            LUMEN_LOG_ERROR( LogLaunchModuleStack, "A registered module descriptor is null" );
            return LaunchFailure( ELaunchStage::Init, ELaunchErrorCode::ModuleCreationFailed,
                                  "A registered module descriptor is null" );
        }

        const Bool bEnabled = ( Desc->IsEnabled == nullptr ) or Desc->IsEnabled( InConfig, InCommandLine );
        if ( not bEnabled )
        {
            for ( const FServiceProvider &Service : Desc->Provides )
            {
                DisabledProviders.push_back( FIndexedProvider{
                    .Id            = Service.Id,
                    .Name          = Service.Name,
                    .QualifiedName = Service.QualifiedName,
                    .OwnerIndex    = InvalidModuleIndex,
                    .OwnerName     = Desc->Name,
                } );
            }
            continue;
        }

        if ( Desc->Create == nullptr ) [[unlikely]]
        {
            LUMEN_LOG_ERROR( LogLaunchModuleStack, "Module '{}' has a null factory", Desc->Name );
            return LaunchFailure( ELaunchStage::Init, ELaunchErrorCode::ModuleCreationFailed, "A module factory is null" );
        }

        const USize OwnerIndex = Enabled.size();
        Enabled.push_back( Desc );

        for ( const FServiceProvider &Service : Desc->Provides )
        {
            if ( const FIndexedProvider *const Existing = FindProvider( ActiveProviders, Service.Id ); Existing != nullptr )
                [[unlikely]]
            {
                if ( Existing->QualifiedName != Service.QualifiedName )
                {
                    LUMEN_LOG_ERROR( LogLaunchModuleStack, "Module type ID hash collision between '{}' and '{}'",
                                     Existing->QualifiedName, Service.QualifiedName );
                    return LaunchFailure( ELaunchStage::Init, ELaunchErrorCode::InvalidConfiguration,
                                          "Module type ID collision detected" );
                }

                LUMEN_LOG_ERROR( LogLaunchModuleStack, "Conflicting providers for '{}': '{}' and '{}'", Service.Name,
                                 Existing->OwnerName, Desc->Name );
                return LaunchFailure( ELaunchStage::Init, ELaunchErrorCode::InvalidConfiguration,
                                      "Multiple enabled modules provide the same module or service" );
            }

            ActiveProviders.push_back( FIndexedProvider{
                .Id            = Service.Id,
                .Name          = Service.Name,
                .QualifiedName = Service.QualifiedName,
                .OwnerIndex    = OwnerIndex,
                .OwnerName     = Desc->Name,
            } );
        }
    }

    if ( Enabled.empty() )
    {
        return {};
    }

    /** 2. Validate required dependencies and assemble DAG edges. */
    const USize ModuleCount = Enabled.size();
    TVector<TVector<USize>> Edges( ModuleCount );
    TVector<USize> InDegree( ModuleCount, 0 );

    for ( USize Index = 0; Index < ModuleCount; ++Index )
    {
        const FModuleDescriptor *const Desc = Enabled[Index];

        for ( const FModuleDep &Req : Desc->Requires )
        {
            const FIndexedProvider *const Provider = FindProvider( ActiveProviders, Req.Id );
            if ( Provider == nullptr or Provider->QualifiedName != Req.QualifiedName ) [[unlikely]]
            {
                if ( const FIndexedProvider *const Disabled = FindProvider( DisabledProviders, Req.Id );
                     Disabled != nullptr and Disabled->QualifiedName == Req.QualifiedName )
                {
                    LUMEN_LOG_ERROR( LogLaunchModuleStack, "Module '{}' requires '{}' which is disabled (provided by '{}')",
                                     Desc->Name, Req.Name, Disabled->OwnerName );
                    return LaunchFailure( ELaunchStage::Init, ELaunchErrorCode::InvalidConfiguration,
                                          "Required module dependency is disabled" );
                }

                LUMEN_LOG_ERROR( LogLaunchModuleStack, "Module '{}' requires missing dependency '{}'", Desc->Name, Req.Name );
                return LaunchFailure( ELaunchStage::Init, ELaunchErrorCode::InvalidConfiguration,
                                      "Required module dependency is missing" );
            }

            if ( not AddUniqueEdge( Edges, InDegree, Provider->OwnerIndex, Index ) ) [[unlikely]]
            {
                LUMEN_LOG_ERROR( LogLaunchModuleStack, "Module '{}' cannot depend on itself ('{}')", Desc->Name, Req.Name );
                return LaunchFailure( ELaunchStage::Init, ELaunchErrorCode::InvalidConfiguration,
                                      "Self-referential module dependency detected" );
            }
        }

        for ( const FModuleDep &Opt : Desc->OptionalRequires )
        {
            const FIndexedProvider *const Provider = FindProvider( ActiveProviders, Opt.Id );
            if ( Provider != nullptr and Provider->QualifiedName == Opt.QualifiedName )
            {
                if ( not AddUniqueEdge( Edges, InDegree, Provider->OwnerIndex, Index ) ) [[unlikely]]
                {
                    LUMEN_LOG_ERROR( LogLaunchModuleStack, "Module '{}' cannot optionally depend on itself", Desc->Name );
                    return LaunchFailure( ELaunchStage::Init, ELaunchErrorCode::InvalidConfiguration,
                                          "Self-referential module dependency detected" );
                }
            }
        }

        for ( const FModuleDep &Bef : Desc->Before )
        {
            const FIndexedProvider *const Target = FindProvider( ActiveProviders, Bef.Id );
            if ( Target != nullptr and Target->QualifiedName == Bef.QualifiedName )
            {
                if ( not AddUniqueEdge( Edges, InDegree, Index, Target->OwnerIndex ) ) [[unlikely]]
                {
                    LUMEN_LOG_ERROR( LogLaunchModuleStack, "Module '{}' cannot be ordered before itself", Desc->Name );
                    return LaunchFailure( ELaunchStage::Init, ELaunchErrorCode::InvalidConfiguration,
                                          "Self-referential module ordering detected" );
                }
            }
        }
    }

    /** 3. Deterministic topological sort (Kahn's algorithm with `(LoadPriority, Name)` tie-breaker). */
    TVector<Bool> Visited( ModuleCount, false );
    TVector<USize> SortedOrder;
    SortedOrder.reserve( ModuleCount );

    for ( USize Step = 0; Step < ModuleCount; ++Step )
    {
        USize Best = InvalidModuleIndex;
        for ( USize Candidate = 0; Candidate < ModuleCount; ++Candidate )
        {
            if ( Visited[Candidate] or InDegree[Candidate] != 0 )
            {
                continue;
            }

            if ( Best == InvalidModuleIndex )
            {
                Best = Candidate;
                continue;
            }

            const FModuleDescriptor *const BestDesc      = Enabled[Best];
            const FModuleDescriptor *const CandidateDesc = Enabled[Candidate];

            if ( CandidateDesc->LoadPriority < BestDesc->LoadPriority or
                 ( CandidateDesc->LoadPriority == BestDesc->LoadPriority and CandidateDesc->Name < BestDesc->Name ) )
            {
                Best = Candidate;
            }
        }

        if ( Best == InvalidModuleIndex )
        {
            break;
        }

        Visited[Best] = true;
        SortedOrder.push_back( Best );

        for ( const USize Dependent : Edges[Best] )
        {
            --InDegree[Dependent];
        }
    }

    if ( SortedOrder.size() != ModuleCount ) [[unlikely]]
    {
        for ( USize Index = 0; Index < ModuleCount; ++Index )
        {
            if ( not Visited[Index] )
            {
                LUMEN_LOG_ERROR( LogLaunchModuleStack, "Circular module dependency involving '{}'", Enabled[Index]->Name );
            }
        }
        return LaunchFailure( ELaunchStage::Init, ELaunchErrorCode::InvalidConfiguration, "Circular module dependency detected" );
    }

    /** 4. Instantiate modules in sorted dependency order; rollback via RAII if any fails. */
    Modules.reserve( ModuleCount );
    Services.reserve( ActiveProviders.size() );
    FixedTickList.reserve( ModuleCount );
    TickList.reserve( ModuleCount );

    TVector<FTickEntry> TickEntries;
    TickEntries.reserve( ModuleCount );

    for ( USize TopoIndex = 0; TopoIndex < SortedOrder.size(); ++TopoIndex )
    {
        const FModuleDescriptor *const Desc = Enabled[SortedOrder[TopoIndex]];

        const FModuleContext Context( InConfig, InCommandLine, Services );
        auto Created = Desc->Create( Context );
        if ( not Created ) [[unlikely]]
        {
            LUMEN_LOG_ERROR( LogLaunchModuleStack, "Module '{}' failed to start", Desc->Name );
            ShutdownAll();
            return MakeUnexpected( Created.Error() );
        }

        TUniquePtr<IEngineModule> Module = std::move( *Created );
        if ( not Module ) [[unlikely]]
        {
            LUMEN_LOG_ERROR( LogLaunchModuleStack, "Module '{}' factory returned null", Desc->Name );
            ShutdownAll();
            return LaunchFailure( ELaunchStage::Init, ELaunchErrorCode::ModuleCreationFailed,
                                  "A module factory returned a null module" );
        }

        Module->ModuleName = Desc->Name;
        LUMEN_LOG_INFO( LogLaunchModuleStack, "Starting module '{}'", Module->GetName() );

        for ( const FServiceProvider &Service : Desc->Provides )
        {
            Services.push_back( FModuleContext::FServiceEntry{
                .Id            = Service.Id,
                .Name          = Service.Name,
                .QualifiedName = Service.QualifiedName,
                .Instance      = Service.Upcast( Module.Get() ),
            } );
        }

        const EModuleTraits Traits = Module->GetTraits();
        if ( HasAnyFlags( Traits, EModuleTraits::FixedTick ) )
        {
            FixedTickList.push_back( Module.Get() );
        }
        if ( HasAnyFlags( Traits, EModuleTraits::Tick ) )
        {
            TickEntries.push_back( FTickEntry{
                .Module    = Module.Get(),
                .Phase     = Desc->TickPhase,
                .TopoIndex = TopoIndex,
            } );
        }

        Modules.push_back( std::move( Module ) );
    }

    /** 5. Build cache-friendly Tick dispatch list sorted by `(TickPhase, TopologicalOrder)`. */
    std::sort( TickEntries.begin(), TickEntries.end(), [] ( const FTickEntry &InLeft, const FTickEntry &InRight ) noexcept -> Bool
    {
        if ( InLeft.Phase != InRight.Phase )
        {
            return static_cast<UInt8>( InLeft.Phase ) < static_cast<UInt8>( InRight.Phase );
        }
        return InLeft.TopoIndex < InRight.TopoIndex;
    } );

    for ( const FTickEntry &Entry : TickEntries )
    {
        TickList.push_back( Entry.Module );
    }

    return {};
}

void Lumen::Launch::FLaunchModuleStack::ShutdownAll () noexcept
{
    FixedTickList.clear();
    TickList.clear();
    Services.clear();

    while ( not Modules.empty() )
    {
        Modules.pop_back();
    }
}

void Lumen::Launch::FLaunchModuleStack::FixedTick ( const Float64 InFixedDeltaSeconds ) noexcept
{
    Algo::ForEach( FixedTickList, [InFixedDeltaSeconds] ( IEngineModule *const Module )
    {
        Module->FixedTick( InFixedDeltaSeconds );
    } );
}

void Lumen::Launch::FLaunchModuleStack::Tick ( const Float64 InDeltaSeconds, const Float64 InAlpha ) noexcept
{
    Algo::ForEach( TickList, [InDeltaSeconds, InAlpha] ( IEngineModule *const Module )
    {
        Module->Tick( InDeltaSeconds, InAlpha );
    } );
}

Lumen::USize Lumen::Launch::FLaunchModuleStack::Num () const noexcept
{
    return Modules.size();
}

void *Lumen::Launch::FLaunchModuleStack::FindServiceRaw ( const FModuleId InId ) const noexcept
{
    for ( const FModuleContext::FServiceEntry &Entry : Services )
    {
        if ( Entry.Id == InId )
        {
            return Entry.Instance;
        }
    }
    return nullptr;
}
