/**
 * @file LaunchModuleStack.inl
 * @brief Template definitions of FLaunchModuleStack.
 */

#pragma once

#include <Launch/LaunchModuleStack.hpp>

template <typename TService> TService *Lumen::Launch::FLaunchModuleStack::FindService () const noexcept
{
    return static_cast<TService *>( FindServiceRaw( Detail::ModuleIdOf<TService>() ) );
}
