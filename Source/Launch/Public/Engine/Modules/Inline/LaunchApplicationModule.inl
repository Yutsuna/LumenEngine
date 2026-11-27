/**
 * @file LaunchApplicationModule.inl
 * @brief Inline and template implementation of LaunchApplicationModule.
 */

#pragma once

#include <Engine/Modules/LaunchApplicationModule.hpp>

#include <type_traits>
#include <utility>

template <typename THandler, typename... TArgs>
Lumen::Launch::FScopedMessageHandler Lumen::Launch::FLaunchApplicationModule::PushMessageHandler ( TArgs &&...InArgs ) noexcept
{
    static_assert( std::is_base_of_v<FGenericApplicationMessageHandler, THandler>,
                   "PushMessageHandler requires a type derived from FGenericApplicationMessageHandler." );

    const TSharedRef<THandler> Handler = MakeShared<THandler>( GetCurrentMessageHandler(), std::forward<TArgs>( InArgs )... );
    return InstallMessageHandler( Handler );
}
