///
/// @file LaunchEngineModules.cpp
/// @brief Module registration table: the composition root of the engine.
///

#include <Engine/LaunchEngineModule.hpp>

#include <Memory/Span.hpp>
#include <Templates/Array.hpp>

Lumen::TSpan<const Lumen::Launch::FModuleDescriptor> Lumen::Launch::GetEngineModules () noexcept
{
    /// Example entry:
    ///     { "Window",  &Window::CreateModule },
    ///     { "RHI",     &RHI::CreateModule },
    ///     { "Physics", &Physics::CreateModule },
    static constexpr TArray<FModuleDescriptor, 0> Modules{};

    return Modules;
}
