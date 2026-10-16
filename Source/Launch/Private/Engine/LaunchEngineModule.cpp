///
/// @file LaunchEngineModule.cpp
/// @brief Module registration table.
///

#include <Engine/LaunchEngineModule.hpp>

#include <Memory/Span.hpp>
#include <Templates/Array.hpp>

namespace Lumen
{

Launch::EModuleTraits Launch::IEngineModule::GetTraits () const noexcept
{
    return EModuleTraits::None;
}

void Launch::IEngineModule::FixedTick ( Float64 __attribute__( ( unused ) ) InFixedDeltaSeconds ) noexcept
{
    /// Empty
}

void Launch::IEngineModule::Tick ( Float64 __attribute__( ( unused ) ) InDeltaSeconds,
                                   Float64 __attribute__( ( unused ) ) InAlpha ) noexcept
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