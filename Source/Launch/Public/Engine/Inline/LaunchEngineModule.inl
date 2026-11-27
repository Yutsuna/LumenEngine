/**
 * @file LaunchEngineModule.inl
 * @brief Inline implementation of LaunchEngineModule.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>
#include <Engine/LaunchEngineModule.hpp>

#include <Meta/Reflection.hpp>

/**
 * Public
 */

constexpr Lumen::FStringView Lumen::Launch::IEngineModule::GetName () const noexcept
{
#if LUMEN_HAS_REFLECTION
    constexpr std::meta::info Class = std::meta::current_class();
    return std::meta::identifier_of( Class );
#else
    return "Unknown";
#endif
}
