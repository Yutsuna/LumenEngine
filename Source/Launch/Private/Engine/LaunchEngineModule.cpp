/**
 * @file LaunchEngineModule.cpp
 * @brief Module base implementation and SIOF-safe intrusive registration table.
 */

#include <Core/CoreDefinitions.hpp>
#include <Engine/LaunchEngineModule.hpp>

/**
 * Private
 */

namespace
{

[[nodiscard]] Lumen::Launch::FModuleDescriptor *&GetMutableModuleHead () noexcept
{
    static Lumen::Launch::FModuleDescriptor *GModuleHead = nullptr;
    return GModuleHead;
}

} // namespace

/**
 * FModuleContext
 */

Lumen::Launch::FModuleContext::FModuleContext ( const FLaunchEngineConfig &InConfig,
                                                const FCommandLine &InCommandLine,
                                                const TSpan<const FServiceEntry> InServices ) noexcept
    : Config( InConfig ), CommandLine( InCommandLine ), Services( InServices )
{
    /** Empty */
}

void *Lumen::Launch::FModuleContext::FindRaw ( const FModuleId InId ) const noexcept
{
    for ( const FServiceEntry &Entry : Services )
    {
        if ( Entry.Id == InId )
        {
            return Entry.Instance;
        }
    }
    return nullptr;
}

const Lumen::Launch::FLaunchEngineConfig &Lumen::Launch::FModuleContext::GetConfig () const noexcept
{
    return Config;
}

const Lumen::Launch::FCommandLine &Lumen::Launch::FModuleContext::GetCommandLine () const noexcept
{
    return CommandLine;
}

/**
 * IEngineModule
 */

Lumen::FStringView Lumen::Launch::IEngineModule::GetName () const noexcept
{
    return ModuleName;
}

Lumen::Launch::EModuleTraits Lumen::Launch::IEngineModule::GetTraits () const noexcept
{
    return EModuleTraits::None;
}

void Lumen::Launch::IEngineModule::FixedTick ( Float64 LUMEN_UNUSED InFixedDeltaSeconds ) noexcept
{
    /** Empty */
}

void Lumen::Launch::IEngineModule::Tick ( Float64 LUMEN_UNUSED InDeltaSeconds, Float64 LUMEN_UNUSED InAlpha ) noexcept
{
    /** Empty */
}

/**
 * Registry
 */

void Lumen::Launch::RegisterModuleDescriptor ( FModuleDescriptor &InDescriptor ) noexcept
{
    if ( InDescriptor.bRegistered ) [[unlikely]]
    {
        return;
    }

    FModuleDescriptor *&Head = GetMutableModuleHead();
    InDescriptor.Next        = Head;
    InDescriptor.bRegistered = true;
    Head                     = &InDescriptor;
}

const Lumen::Launch::FModuleDescriptor *Lumen::Launch::GetRegisteredModuleHead () noexcept
{
    return GetMutableModuleHead();
}

Lumen::TVector<const Lumen::Launch::FModuleDescriptor *> Lumen::Launch::CollectRegisteredModules () noexcept
{
    USize Count = 0;
    for ( const FModuleDescriptor *Current = GetRegisteredModuleHead(); Current != nullptr; Current = Current->Next )
    {
        ++Count;
    }

    TVector<const FModuleDescriptor *> Result;
    Result.reserve( Count );

    for ( const FModuleDescriptor *Current = GetRegisteredModuleHead(); Current != nullptr; Current = Current->Next )
    {
        Result.push_back( Current );
    }

    return Result;
}
