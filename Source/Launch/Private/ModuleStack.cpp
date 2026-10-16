///
/// @file ModuleStack.cpp
/// @brief FModuleStack implementation.
///

#include <Launch/ModuleStack.hpp>

#include <Logging/Logger.hpp>

#include <utility>

namespace
{

LUMEN_LOG_DEFINE_CATEGORY( LogModuleStack, "ModuleStack" )

}

namespace Lumen
{
Launch::FModuleStack::FModuleStack ( FModuleStack &&Other ) noexcept
    : Modules( std::move( Other.Modules ) ), FixedTickList( std::move( Other.FixedTickList ) ),
      TickList( std::move( Other.TickList ) )
{
    Other.Modules.clear();
    Other.FixedTickList.clear();
    Other.TickList.clear();
}

Launch::FModuleStack::~FModuleStack () noexcept
{
    ShutdownAll();
}

Launch::TLaunchResult<void> Launch::FModuleStack::StartAll ( const TSpan<const FModuleDescriptor> InDescriptors,
                                                             const FLaunchEngineConfig &InConfig ) noexcept
{
    /// Reserve once: the push_back calls below can no longer allocate.
    Modules.reserve( Modules.size() + InDescriptors.size() );
    FixedTickList.reserve( FixedTickList.size() + InDescriptors.size() );
    TickList.reserve( TickList.size() + InDescriptors.size() );

    for ( const FModuleDescriptor &Descriptor : InDescriptors )
    {
        if ( Descriptor.Create == nullptr ) [[unlikely]]
        {
            LUMEN_LOG_ERROR( LogModuleStack, "Module '{}' has no factory", Descriptor.Name );
            return LaunchFailure( ELaunchStage::Init, ELaunchErrorCode::ModuleCreationFailed,
                                  "A module descriptor has no factory" );
        }

        auto Created = Descriptor.Create( InConfig );
        if ( not Created ) [[unlikely]]
        {
            /// The error message is a static literal and cannot carry the module name: log it here.
            LUMEN_LOG_ERROR( LogModuleStack, "Module '{}' failed to start", Descriptor.Name );
            return MakeUnexpected( Created.Error() );
        }

        TUniquePtr<IEngineModule> Module = std::move( *Created );
        if ( not Module ) [[unlikely]]
        {
            LUMEN_LOG_ERROR( LogModuleStack, "Module '{}': factory returned a null module", Descriptor.Name );
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

void Launch::FModuleStack::ShutdownAll () noexcept
{
    FixedTickList.clear();
    TickList.clear();

    while ( not Modules.empty() )
    {
        Modules.pop_back();
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
void Launch::FModuleStack::FixedTick ( const Float64 InFixedDeltaSeconds ) noexcept
{
    for ( IEngineModule *const Module : FixedTickList )
    {
        Module->FixedTick( InFixedDeltaSeconds );
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
void Launch::FModuleStack::Tick ( const Float64 InDeltaSeconds, const Float64 InAlpha ) noexcept
{
    for ( IEngineModule *const Module : TickList )
    {
        Module->Tick( InDeltaSeconds, InAlpha );
    }
}

} // namespace Lumen
