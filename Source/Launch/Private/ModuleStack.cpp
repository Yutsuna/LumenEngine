///
/// @file ModuleStack.cpp
/// @brief FModuleStack implementation.
///

#include <Launch/ModuleStack.hpp>

#include <utility>

namespace Lumen
{
namespace Launch
{
    FModuleStack::FModuleStack ( FModuleStack &&Other ) noexcept
        : Modules( std::move( Other.Modules ) ), FixedTickList( std::move( Other.FixedTickList ) ),
          TickList( std::move( Other.TickList ) )
    {
        Other.Modules.clear();
        Other.FixedTickList.clear();
        Other.TickList.clear();
    }

    FModuleStack::~FModuleStack () noexcept
    {
        ShutdownAll();
    }

    TLaunchResult<void> FModuleStack::StartAll ( const TSpan<const FModuleDescriptor> InDescriptors,
                                                 const FLaunchEngineConfig &InConfig ) noexcept
    {
        /// Reserve once: the push_back calls below can no longer allocate.
        Modules.reserve( Modules.size() + InDescriptors.size() );
        FixedTickList.reserve( FixedTickList.size() + InDescriptors.size() );
        TickList.reserve( TickList.size() + InDescriptors.size() );

        for ( const FModuleDescriptor &Descriptor : InDescriptors )
        {
            auto Created = Descriptor.Create( InConfig );
            if ( not Created ) [[unlikely]]
            {
                return MakeUnexpected( Created.Error() );
            }

            TUniquePtr<IEngineModule> Module = std::move( *Created );
            if ( not Module ) [[unlikely]]
            {
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

    void FModuleStack::ShutdownAll () noexcept
    {
        FixedTickList.clear();
        TickList.clear();

        while ( not Modules.empty() )
        {
            Modules.pop_back(); /// Destroys the last element first.
        }
    }

    void FModuleStack::FixedTick ( const Float64 InFixedDeltaSeconds ) noexcept
    {
        for ( IEngineModule *const Module : FixedTickList )
        {
            Module->FixedTick( InFixedDeltaSeconds );
        }
    }

    void FModuleStack::Tick ( const Float64 InDeltaSeconds, const Float64 InAlpha ) noexcept
    {
        for ( IEngineModule *const Module : TickList )
        {
            Module->Tick( InDeltaSeconds, InAlpha );
        }
    }
} // namespace Launch
} // namespace Lumen
