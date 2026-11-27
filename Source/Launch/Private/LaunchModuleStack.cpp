/**
 * @file LaunchModuleStack.cpp
 * @brief FLaunchModuleStack implementation.
 */

#include <Launch/LaunchModuleStack.hpp>

#include <Logging/Logger.hpp>
#include <Algo/ForEach.hpp>

#include <utility>

/**
 * Private
 */

namespace
{

LUMEN_LOG_DEFINE_CATEGORY( LogLaunchModuleStack, "LaunchModuleStack" )

}


/**
 * Public
 */

Lumen::Launch::FLaunchModuleStack::FLaunchModuleStack ( FLaunchModuleStack &&Other ) noexcept
    : Modules( std::move( Other.Modules ) ), FixedTickList( std::move( Other.FixedTickList ) ),
      TickList( std::move( Other.TickList ) )
{
    Other.Modules.clear();
    Other.FixedTickList.clear();
    Other.TickList.clear();
}

Lumen::Launch::FLaunchModuleStack::~FLaunchModuleStack () noexcept
{
    ShutdownAll();
}

Lumen::Launch::TLaunchResult<void> Lumen::Launch::FLaunchModuleStack::StartAll ( const TSpan<const FModuleDescriptor> InDescriptors,
                                                                   const FLaunchEngineConfig &InConfig ) noexcept
{
    /** Reserve once: the push_back calls below can no longer allocate. */
    Modules.reserve( Modules.size() + InDescriptors.size() );
    FixedTickList.reserve( FixedTickList.size() + InDescriptors.size() );
    TickList.reserve( TickList.size() + InDescriptors.size() );

    for ( const FModuleDescriptor &Descriptor : InDescriptors )
    {
        LUMEN_LOG_INFO( LogLaunchModuleStack, "Starting module '{}'", Descriptor.Name );

        if ( Descriptor.Create == nullptr ) [[unlikely]]
        {
            LUMEN_LOG_ERROR( LogLaunchModuleStack, "Module '{}' has no factory", Descriptor.Name );
            return LaunchFailure( ELaunchStage::Init, ELaunchErrorCode::ModuleCreationFailed,
                                  "A module descriptor has no factory" );
        }

        auto Created = Descriptor.Create( InConfig );
        if ( not Created ) [[unlikely]]
        {
            /** The error message is a static literal and cannot carry the module name: log it here. */
            LUMEN_LOG_ERROR( LogLaunchModuleStack, "Module '{}' failed to start", Descriptor.Name );
            return MakeUnexpected( Created.Error() );
        }

        TUniquePtr<IEngineModule> Module = std::move( *Created );
        if ( not Module ) [[unlikely]]
        {
            LUMEN_LOG_ERROR( LogLaunchModuleStack, "Module '{}': factory returned a null module", Descriptor.Name );
            return LaunchFailure( ELaunchStage::Init, ELaunchErrorCode::ModuleCreationFailed,
                                  "A module factory returned a null module" );
        }

        const EModuleTraits Traits = Module->GetTraits();
        if ( HasAnyFlags( Traits, EModuleTraits::FixedTick ) )
        {
            FixedTickList.push_back( Module.Get() );
        }
        if ( HasAnyFlags( Traits, EModuleTraits::Tick ) )
        {
            TickList.push_back( Module.Get() );
        }

        Modules.push_back( std::move( Module ) );
    }

    return {};
}

void Lumen::Launch::FLaunchModuleStack::ShutdownAll () noexcept
{
    FixedTickList.clear();
    TickList.clear();

    while ( not Modules.empty() )
    {
        Modules.pop_back();
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
void Lumen::Launch::FLaunchModuleStack::FixedTick ( const Float64 InFixedDeltaSeconds ) noexcept
{
    Algo::ForEach( FixedTickList, [InFixedDeltaSeconds](IEngineModule *const Module)
    {
        Module->FixedTick( InFixedDeltaSeconds );
    });
}

// ReSharper disable once CppMemberFunctionMayBeConst
void Lumen::Launch::FLaunchModuleStack::Tick ( const Float64 InDeltaSeconds, const Float64 InAlpha ) noexcept
{
    Algo::ForEach( TickList, [InDeltaSeconds, InAlpha](IEngineModule *const Module)
    {
        Module->Tick( InDeltaSeconds, InAlpha );
    });
}
