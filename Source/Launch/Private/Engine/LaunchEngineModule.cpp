///
/// @file LaunchEngineModule.cpp
/// @brief Module registration table.
///

#include <Core/CoreDefinitions.hpp>
#include <Engine/LaunchEngineModule.hpp>

#include <Memory/Span.hpp>
#include <Templates/Array.hpp>

namespace Lumen
{

Launch::EModuleTraits Launch::IEngineModule::GetTraits () const noexcept
{
    return EModuleTraits::None;
}

void Launch::IEngineModule::FixedTick ( Float64 LUMEN_UNUSED InFixedDeltaSeconds ) noexcept
{
    /// Empty
}

void Launch::IEngineModule::Tick ( Float64 LUMEN_UNUSED InDeltaSeconds,
                                   Float64 LUMEN_UNUSED InAlpha ) noexcept
{
    /// Empty
}

TSpan<const Launch::FModuleDescriptor> Launch::GetEngineModules () noexcept
{
    /// Example entry:
    ///     { "Window",  &Window::CreateModule },
    ///     { "RHI",     &RHI::CreateModule },
    ///     { "Physics", &Physics::CreateModule },
    static constexpr TArray<FModuleDescriptor, 0> Modules{};

    return Modules;
}

} // namespace Lumen