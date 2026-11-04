///
/// @file LaunchEngineModule.cpp
/// @brief Module registration table.
///

#include <Core/CoreDefinitions.hpp>
#include <Engine/LaunchEngineModule.hpp>

#include <Memory/Span.hpp>
#include <Templates/Array.hpp>

#include <Engine/Modules/LaunchApplicationModule.hpp>
#include <Engine/Modules/LaunchWindowModule.hpp>

Lumen::Launch::EModuleTraits Lumen::Launch::IEngineModule::GetTraits () const noexcept
{
    return EModuleTraits::None;
}

void Lumen::Launch::IEngineModule::FixedTick ( Float64 LUMEN_UNUSED InFixedDeltaSeconds ) noexcept
{
    /// Empty
}

void Lumen::Launch::IEngineModule::Tick ( Float64 LUMEN_UNUSED InDeltaSeconds, Float64 LUMEN_UNUSED InAlpha ) noexcept
{
    /// Empty
}

Lumen::TSpan<const Lumen::Launch::FModuleDescriptor> Lumen::Launch::GetEngineModules () noexcept
{
    static constexpr TArray<FModuleDescriptor, 2> Modules = {
        FModuleDescriptor{ .Name = "Application", .Create = &MakeApplicationModule },
        FModuleDescriptor{ .Name = "Window", .Create = &MakeWindowModule },
    };

    return Modules;
}
